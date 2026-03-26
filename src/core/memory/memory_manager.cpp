#ifndef IDEAM_CORE_MEMORY_MANAGER_CPP
#define IDEAM_CORE_MEMORY_MANAGER_CPP

#include "memory_manager.h"
#include <godot_cpp/classes/rd_uniform.hpp>
#include <cstdlib>
#include <cstring>

namespace ideam::core {

MemoryManager::MemoryManager() {
    grant_pool.reserve(128);
}

MemoryManager::~MemoryManager() {
    _free_master_block();
    registry.clear();

    // Cleanup GPU RIDs
    for (auto& state : buffer_states) {
        if (rd && state.gpu_rid.is_valid()) {
            rd->free_rid(state.gpu_rid);
        }
    }

    for (MemoryGrant* grant : grant_pool) {
        delete grant;
    }
    grant_pool.clear();
}

void MemoryManager::register_buffer(IMemoryBuffer* p_buffer) {
    if (!p_buffer) return;
    
    std::lock_guard<std::mutex> lock(manager_mutex);
    auto it = std::find(registry.begin(), registry.end(), p_buffer);
    if (it == registry.end()) {
        registry.push_back(p_buffer);
        _recalculate_layout();
    }
}

void MemoryManager::unregister_buffer(IMemoryBuffer* p_buffer) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    auto it = std::find(registry.begin(), registry.end(), p_buffer);
    if (it != registry.end()) {
        registry.erase(it);
        
        // Remove associated buffer state and free GPU RID
        uint32_t id = p_buffer->get_buffer_id();
        for (auto it_state = buffer_states.begin(); it_state != buffer_states.end(); ++it_state) {
            if (it_state->buffer_id == id) {
                if (rd && it_state->gpu_rid.is_valid()) {
                    rd->free_rid(it_state->gpu_rid);
                }
                buffer_states.erase(it_state);
                break;
            }
        }

        _recalculate_layout();
    }
}

void MemoryManager::request_reallocation() {
    std::lock_guard<std::mutex> lock(manager_mutex);
    _recalculate_layout();
}

MemoryGrant* MemoryManager::request_grant(const std::vector<GrantPart>& p_requirements, bool p_needs_gpu) {
    std::lock_guard<std::mutex> lock(manager_mutex);

    // 1. Collision Detection Phase
    for (const auto& req : p_requirements) {
        BufferState* state = _get_or_create_state(req.buffer_id);
        
        if (req.access_mode == BufferAccessMode::WRITE || req.access_mode == BufferAccessMode::READ_WRITE) {
            if (state->has_writer || state->active_readers > 0) {
                if (CollisionUtils::has_intersection(state->write_mask, req.selection->bitset)) {
                    return nullptr;
                }
            }
        } else if (req.access_mode == BufferAccessMode::READ) {
            if (state->has_writer) {
                if (CollisionUtils::has_intersection(state->write_mask, req.selection->bitset)) {
                    return nullptr;
                }
            }
        }
    }

    // 2. Commitment & GPU Mirroring Phase
    godot::Array uniforms;

    for (uint32_t i = 0; i < p_requirements.size(); ++i) {
        const auto& req = p_requirements[i];
        BufferState* state = _get_or_create_state(req.buffer_id);
        
        // Handle GPU on-demand allocation and sync
        if (p_needs_gpu && rd) {
            IMemoryBuffer* buf = _find_buffer(req.buffer_id);
            if (buf) {
                // On-demand allocation
                if (!state->gpu_rid.is_valid()) {
                    state->gpu_rid = rd->storage_buffer_create(buf->get_capacity());
                    state->dirty_gpu = true; 
                }
                
                // Just-in-time upload
                if (state->dirty_gpu) {
                    _sync_to_vram(*state, buf);
                }

                // Prepare Godot Uniform for this binding
                godot::Ref<godot::RDUniform> u;
                u.instantiate();
                u->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
                u->set_binding(i); // Binding matches GrantPart order
                u->add_id(state->gpu_rid);
                uniforms.push_back(u);
            }
        }

        if (req.access_mode == BufferAccessMode::WRITE || req.access_mode == BufferAccessMode::READ_WRITE) {
            state->has_writer = true;
            CollisionUtils::apply_union(state->write_mask, req.selection->bitset);
            
            // If CPU writes, GPU version is now stale. If GPU writes, CPU version is now stale.
            if (p_needs_gpu) state->dirty_cpu = true;
            else state->dirty_gpu = true;
        } else {
            state->active_readers++;
        }
    }

    // 3. Bake UniformSet if needed
    godot::RID us_rid;
    if (rd && !uniforms.is_empty()) {
        us_rid = rd->uniform_set_create(uniforms, godot::RID(), 0);
    }

    // 4. Grant Allocation
    MemoryGrant* grant = nullptr;
    if (!grant_pool.empty()) {
        grant = grant_pool.back();
        grant_pool.pop_back();
    } else {
        grant = new MemoryGrant();
    }

    grant->set_parts(p_requirements, current_version, &current_version, us_rid);
    return grant;
}

void MemoryManager::release_grant(MemoryGrant* p_grant) {
    if (!p_grant) return;

    std::lock_guard<std::mutex> lock(manager_mutex);

    // Cleanup GPU resource for this work unit
    if (rd && p_grant->get_uniform_set_rid().is_valid()) {
        rd->free_rid(p_grant->get_uniform_set_rid());
    }

    for (const auto& part : p_grant->get_parts()) {
        BufferState* state = _get_or_create_state(part.buffer_id);
        if (part.access_mode == BufferAccessMode::WRITE || part.access_mode == BufferAccessMode::READ_WRITE) {
            CollisionUtils::apply_difference(state->write_mask, part.selection->bitset);
            state->has_writer = false; 
        } else {
            if (state->active_readers > 0) state->active_readers--;
        }
    }

    p_grant->_wipe();
    grant_pool.push_back(p_grant);
}

void MemoryManager::_recalculate_layout() {
    current_version++; 

    if (registry.empty()) {
        _free_master_block();
        return;
    }

    size_t new_total_size = 0;
    for (IMemoryBuffer* buf : registry) {
        new_total_size = MemoryUtilities::align_to(new_total_size, buf->get_alignment());
        new_total_size += buf->get_capacity();
    }

    uint8_t* new_block = static_cast<uint8_t*>(std::malloc(new_total_size));
    if (!new_block) return;

    size_t current_offset = 0;
    for (IMemoryBuffer* buf : registry) {
        current_offset = MemoryUtilities::align_to(current_offset, buf->get_alignment());
        
        uint8_t* old_ptr = buf->get_raw_ptr();
        uint8_t* new_ptr = new_block + current_offset;

        if (old_ptr && buf->get_used_size() > 0) {
            std::memcpy(new_ptr, old_ptr, buf->get_used_size());
        }

        buf->rebase(new_block, current_offset, buf->get_capacity());

        // Update GPU resource if it already exists
        BufferState* state = _get_or_create_state(buf->get_buffer_id());
        if (rd && state->gpu_rid.is_valid()) {
            rd->free_rid(state->gpu_rid);
            state->gpu_rid = rd->storage_buffer_create(buf->get_capacity());
            state->dirty_gpu = true; // Force sync on next GPU use
        }

        current_offset += buf->get_capacity();
    }

    _free_master_block();
    master_block = new_block;
    master_block_size = new_total_size;
}

void MemoryManager::_sync_to_vram(BufferState& p_state, IMemoryBuffer* p_buffer) {
    if (!rd || !p_state.gpu_rid.is_valid() || !p_buffer) return;
    
    size_t size = p_buffer->get_capacity();
    uint8_t* raw_ptr = p_buffer->get_raw_ptr();

    if (raw_ptr && size > 0) {
        // Create a PackedByteArray and resize it to fit the buffer data
        godot::PackedByteArray data;
        data.resize(size);
        
        // Copy the raw master_block data into the Godot-managed array
        std::memcpy(data.ptrw(), raw_ptr, size);
        
        // Now Godot can accept the data
        rd->buffer_update(p_state.gpu_rid, 0, size, data);
    }
    
    p_state.dirty_gpu = false;
}

IMemoryBuffer* MemoryManager::_find_buffer(uint32_t p_buffer_id) {
    for (auto buf : registry) {
        if (buf->get_buffer_id() == p_buffer_id) return buf;
    }
    return nullptr;
}

void MemoryManager::_free_master_block() {
    if (master_block) {
        std::free(master_block);
        master_block = nullptr;
        master_block_size = 0;
    }
}

MemoryManager::BufferState* MemoryManager::_get_or_create_state(uint32_t p_buffer_id) {
    for (auto& state : buffer_states) {
        if (state.buffer_id == p_buffer_id) return &state;
    }
    
    buffer_states.emplace_back();
    BufferState& newState = buffer_states.back();
    newState.buffer_id = p_buffer_id;
    return &newState;
}

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_MANAGER_CPP