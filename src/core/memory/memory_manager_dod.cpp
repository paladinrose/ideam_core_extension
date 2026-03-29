#include "memory_manager_dod.h"
#include <godot_cpp/classes/rd_uniform.hpp>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace ideam::core {

// Fallback alignment for compilers that don't support C++17 hardware_destructive_interference_size yet
#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
    constexpr size_t CACHE_LINE_SIZE = 64; 
#endif

MemoryManagerDOD::MemoryManagerDOD(size_t p_initial_capacity) 
    : master_capacity(p_initial_capacity) {
        
    // Hardware-Aware Contiguous Allocation to prevent cross-lane cache thrashing.
#if defined(_MSC_VER)
    master_block_ptr = static_cast<uint8_t*>(_aligned_malloc(master_capacity, CACHE_LINE_SIZE));
#else
    master_block_ptr = static_cast<uint8_t*>(std::aligned_alloc(CACHE_LINE_SIZE, master_capacity));
#endif

    std::memset(master_block_ptr, 0, master_capacity);

    // Pre-reserve capacities to prevent vector reallocations invalidating MemoryBufferPOD pointers.
    buffers.reserve(1024);
    buffer_gpu_rids.reserve(1024);
    buffer_dirty_flags.reserve(1024);
    id_to_index.reserve(1024);
    data_to_shadow_map.reserve(1024);
    active_write_masks.reserve(1024);

    _initialize_system_buffers();
}

MemoryManagerDOD::~MemoryManagerDOD() {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock); 
    
    if (rd) {
        for (auto& rid : buffer_gpu_rids) {
            if (rid.is_valid()) rd->free_rid(rid);
        }
    }
    
    if (master_block_ptr) {
#if defined(_MSC_VER)
        _aligned_free(master_block_ptr);
#else
        std::free(master_block_ptr);
#endif
    }

    // Clean up Page Tables for PAGED buffers
    for (auto& buf : buffers) {
        if (buf.layout_type == BufferLayoutType::PAGED && buf.extra.paged.table_ptr) {
            std::free(buf.extra.paged.table_ptr);
        }
    }
}

void MemoryManagerDOD::_initialize_system_buffers() {
    // Buffer 0: Internal GPU Command Ring (1MB for sync instructions)
    system_command_ring_id = create_buffer(BufferLayoutType::RING, 1024 * 1024, 64);
}

uint32_t MemoryManagerDOD::create_buffer(BufferLayoutType p_layout, size_t p_size_bytes, uint32_t p_alignment) {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock);

    size_t padding = MemoryUtilities::align_to(master_used, p_alignment) - master_used;
    if (master_used + padding + p_size_bytes > master_capacity) {
        return 0xFFFFFFFF; 
    }

    master_used += padding;
    uint32_t id = static_cast<uint32_t>(buffers.size()) + 1; // 1-based IDs

    MemoryBufferPOD buf{};
    buf.buffer_id = id;
    buf.version = 1;
    buf.layout_type = p_layout;
    buf.master_block_ptr = master_block_ptr;
    buf.memory_offset = master_used;
    buf.capacity_bytes = p_size_bytes;
    buf.alignment_requirement = p_alignment;
    buf.lifecycle = BufferLifecycleState::IDLE;

    if (p_layout == BufferLayoutType::RING) {
        buf.extra.ring.head_offset = 0;
        buf.extra.ring.tail_offset = 0;
        buf.extra.ring.is_full = false;
    }

    uint32_t internal_idx = static_cast<uint32_t>(buffers.size());
    buffers.push_back(buf);
    buffer_gpu_rids.push_back(godot::RID());
    buffer_dirty_flags.push_back(0); 
    
    // Flat Array Expansion
    if (id >= id_to_index.size()) id_to_index.resize(id + 1, 0xFFFFFFFF);
    id_to_index[id] = internal_idx;
    
    if (id >= active_write_masks.size()) active_write_masks.resize(id + 1);

    master_used += p_size_bytes;
    return id;
}

uint32_t MemoryManagerDOD::create_shadowed_buffer(BufferLayoutType p_layout, size_t p_size_bytes, int64_t p_max_elements, SelectionMode p_selection_mode) {
    uint32_t data_id = create_buffer(p_layout, p_size_bytes, 64);
    if (data_id == 0xFFFFFFFF) return 0xFFFFFFFF;

    size_t selection_data_size = (p_selection_mode == SelectionMode::DENSE) ? 
        ((p_max_elements + 63) / 64) * sizeof(uint64_t) : p_max_elements * sizeof(int64_t);

    size_t meta_soa_size = (p_max_elements * sizeof(uint32_t)) + (p_max_elements * sizeof(int64_t)) + 
                           (p_max_elements * sizeof(uint32_t)) + (p_max_elements * sizeof(uint8_t));

    uint32_t shadow_id = create_buffer(BufferLayoutType::FLAT, selection_data_size + meta_soa_size + 256, 64);
    if (shadow_id != 0xFFFFFFFF) {
        std::unique_lock<std::shared_mutex> lock(manager_rw_lock);
        
        if (data_id >= data_to_shadow_map.size()) data_to_shadow_map.resize(data_id + 1, 0xFFFFFFFF);
        data_to_shadow_map[data_id] = shadow_id;
        
        buffers[id_to_index[shadow_id]].max_elements = p_max_elements;
        buffers[id_to_index[data_id]].max_elements = p_max_elements;

        // Pre-allocate the collision mask to ensure atomic_ref operations don't segfault on uninitialized space.
        size_t bitset_word_count = (p_max_elements + 63) / 64;
        active_write_masks[data_id].resize(bitset_word_count, 0);
    }
    return data_id;
}

void MemoryManagerDOD::_resolve_selection_pointers(uint32_t p_data_buffer_id, MemoryBufferSelectionPOD& r_selection) {
    if (p_data_buffer_id >= data_to_shadow_map.size() || data_to_shadow_map[p_data_buffer_id] == 0xFFFFFFFF) return;

    MemoryBufferPOD& s_buf = buffers[id_to_index[data_to_shadow_map[p_data_buffer_id]]];
    uint8_t* base = s_buf.master_block_ptr + s_buf.memory_offset;
    int64_t count = s_buf.max_elements;

    size_t selection_size = (r_selection.mode == SelectionMode::DENSE) ? 
        ((count + 63) / 64) * sizeof(uint64_t) : count * sizeof(int64_t);

    if (r_selection.mode == SelectionMode::DENSE) r_selection.data.bitset = reinterpret_cast<uint64_t*>(base);
    else r_selection.data.indices = reinterpret_cast<int64_t*>(base);

    r_selection.group_masks  = reinterpret_cast<uint32_t*>(base + selection_size);
    r_selection.partition_ids = reinterpret_cast<int64_t*>(base + selection_size + (count * 4));
    r_selection.version_tags  = reinterpret_cast<uint32_t*>(base + selection_size + (count * 12));
    r_selection.lod_levels    = reinterpret_cast<uint8_t*>(base + selection_size + (count * 16));
    r_selection.target_buffer_id = p_data_buffer_id;
    r_selection.capacity = count;
    r_selection.manager_version = global_version;
}

bool MemoryManagerDOD::_check_and_apply_selection_to_mask(const MemoryBufferSelectionPOD& p_selection, uint64_t* p_mask, bool p_add) {
    bool conflict = false;

    if (p_selection.mode == SelectionMode::DENSE && p_selection.data.bitset) {
        size_t words = (p_selection.capacity + 63) / 64;
        for (size_t i = 0; i < words; ++i) {
            uint64_t target_bits = p_selection.data.bitset[i];
            if (target_bits == 0) continue;

            std::atomic_ref<uint64_t> atomic_word(p_mask[i]);

            if (p_add) {
                uint64_t expected = atomic_word.load(std::memory_order_relaxed);
                do {
                    if (expected & target_bits) return true; // Collision
                } while (!atomic_word.compare_exchange_weak(expected, expected | target_bits, std::memory_order_acquire, std::memory_order_relaxed));
            } else {
                atomic_word.fetch_and(~target_bits, std::memory_order_release);
            }
        }
    } else if (p_selection.mode == SelectionMode::SPARSE && p_selection.data.indices) {
        // Spatial Check Phase
        if (p_add) {
            for (int64_t i = 0; i < p_selection.element_count; ++i) {
                int64_t idx = p_selection.data.indices[i];
                std::atomic_ref<uint64_t> atomic_word(p_mask[idx >> 6]);
                if (atomic_word.load(std::memory_order_relaxed) & (1ULL << (idx & 63))) return true; 
            }
        }
        // Application Phase
        for (int64_t i = 0; i < p_selection.element_count; ++i) {
            int64_t idx = p_selection.data.indices[i];
            std::atomic_ref<uint64_t> atomic_word(p_mask[idx >> 6]);
            if (p_add) {
                atomic_word.fetch_or(1ULL << (idx & 63), std::memory_order_release);
            } else {
                atomic_word.fetch_and(~(1ULL << (idx & 63)), std::memory_order_release);
            }
        }
    }
    return conflict;
}

void MemoryManagerDOD::configure_tiled_soa(uint32_t p_id, uint32_t p_elements_per_tile) {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock);
    if (p_id >= id_to_index.size() || id_to_index[p_id] == 0xFFFFFFFF) return;

    MemoryBufferPOD& buf = buffers[id_to_index[p_id]];
    buf.layout_type = BufferLayoutType::TILED_SOA;
    buf.extra.tiled.elements_per_tile = p_elements_per_tile;
    buf.version++;
}

void MemoryManagerDOD::configure_paged(uint32_t p_id, uint32_t p_page_size_bytes) {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock);
    if (p_id >= id_to_index.size() || id_to_index[p_id] == 0xFFFFFFFF) return;

    MemoryBufferPOD& buf = buffers[id_to_index[p_id]];
    buf.layout_type = BufferLayoutType::PAGED;
    buf.extra.paged.page_size_bytes = p_page_size_bytes;
    buf.extra.paged.page_count = static_cast<uint32_t>(buf.capacity_bytes / p_page_size_bytes);
    
    size_t table_size = buf.extra.paged.page_count * sizeof(uint8_t*);
    buf.extra.paged.table_ptr = static_cast<uint8_t**>(std::malloc(table_size));
    
    uint8_t* buffer_start = buf.master_block_ptr + buf.memory_offset;
    for (uint32_t i = 0; i < buf.extra.paged.page_count; ++i) {
        buf.extra.paged.table_ptr[i] = buffer_start + (i * p_page_size_bytes);
    }
    buf.version++;
}

void MemoryManagerDOD::configure_buffer_columns(uint32_t p_id, const std::vector<ColumnMetadata>& p_columns) {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock);
    if (p_id >= id_to_index.size() || id_to_index[p_id] == 0xFFFFFFFF) return;

    MemoryBufferPOD& buf = buffers[id_to_index[p_id]];
    buf.column_count = std::min(static_cast<uint32_t>(p_columns.size()), static_cast<uint32_t>(MAX_BUFFER_COLUMNS));
    
    size_t current_offset = 0;
    uint32_t total_element_size = 0;

    for (uint32_t i = 0; i < buf.column_count; ++i) {
        buf.columns[i] = p_columns[i];
        total_element_size += p_columns[i].type_size;
        
        if (buf.layout_type == BufferLayoutType::SOA) {
            buf.columns[i].offset = current_offset;
            current_offset += (buf.max_elements * p_columns[i].type_size);
        } else if (buf.layout_type == BufferLayoutType::AOS) {
            buf.columns[i].offset = (i > 0) ? (buf.columns[i-1].offset + buf.columns[i-1].type_size) : 0;
        }
    }

    if (buf.layout_type == BufferLayoutType::SOA && current_offset > buf.capacity_bytes) return;

    if (buf.layout_type == BufferLayoutType::AOS) {
        buf.element_stride = total_element_size;
        if ((size_t)buf.element_stride * buf.max_elements > buf.capacity_bytes) return;
    } else if (buf.layout_type == BufferLayoutType::TILED_SOA) {
        uint32_t tile_size_bytes = 0;
        for (uint32_t i = 0; i < buf.column_count; ++i) {
            buf.columns[i].offset = tile_size_bytes;
            tile_size_bytes += (buf.extra.tiled.elements_per_tile * buf.columns[i].type_size);
        }
        buf.extra.tiled.tile_stride_bytes = tile_size_bytes;
        size_t total_tiles = (buf.max_elements + buf.extra.tiled.elements_per_tile - 1) / buf.extra.tiled.elements_per_tile;
        if (total_tiles * tile_size_bytes > buf.capacity_bytes) return;
    }
    buf.version++; 
}

bool MemoryManagerDOD::ring_push(uint32_t p_id, const void* p_data, size_t p_size) {
    std::lock_guard<std::mutex> lock(command_ring_mutex); // Fine-grained lock for streaming buffers.
    if (p_id >= id_to_index.size() || id_to_index[p_id] == 0xFFFFFFFF) return false;
    
    MemoryBufferPOD& buf = buffers[id_to_index[p_id]];
    if (buf.layout_type != BufferLayoutType::RING) return false;

    size_t next_head = (buf.extra.ring.head_offset + p_size) % buf.capacity_bytes;
    if (next_head == buf.extra.ring.tail_offset) {
        buf.extra.ring.is_full = true;
        return false;
    }

    uint8_t* buffer_start = buf.master_block_ptr + buf.memory_offset;
    std::memcpy(buffer_start + buf.extra.ring.head_offset, p_data, p_size);
    
    buf.extra.ring.head_offset = next_head;
    buf.extra.ring.is_full = false;
    return true;
}

bool MemoryManagerDOD::ring_pop(uint32_t p_id, void* r_dest, size_t p_size) {
    std::lock_guard<std::mutex> lock(command_ring_mutex);
    if (p_id >= id_to_index.size() || id_to_index[p_id] == 0xFFFFFFFF) return false;
    
    MemoryBufferPOD& buf = buffers[id_to_index[p_id]];
    if (buf.layout_type != BufferLayoutType::RING) return false;

    if (buf.extra.ring.tail_offset == buf.extra.ring.head_offset && !buf.extra.ring.is_full) {
        return false; // Empty
    }

    uint8_t* buffer_start = buf.master_block_ptr + buf.memory_offset;
    std::memcpy(r_dest, buffer_start + buf.extra.ring.tail_offset, p_size);
    
    buf.extra.ring.tail_offset = (buf.extra.ring.tail_offset + p_size) % buf.capacity_bytes;
    buf.extra.ring.is_full = false;
    return true;
}

bool MemoryManagerDOD::_bake_grant_core(GrantPartPOD* r_parts, uint32_t max_parts, uint32_t& r_part_count, uint64_t& r_uniform_handle, std::span<const GrantPartPOD> p_requirements, bool p_needs_gpu) {
    // Shared Lock: Permits thousands of spatial worker threads to bake simultaneously without blocking.
    std::shared_lock<std::shared_mutex> lock(manager_rw_lock);

    godot::Array uniforms;

    for (size_t i = 0; i < p_requirements.size(); ++i) {
        const GrantPartPOD& req = p_requirements[i];
        if (req.buffer_id >= id_to_index.size() || id_to_index[req.buffer_id] == 0xFFFFFFFF) return false;
        
        uint32_t idx = id_to_index[req.buffer_id];
        MemoryBufferPOD& buf = buffers[idx];
        MemoryBufferSelectionPOD selection = req.selection;
        _resolve_selection_pointers(req.buffer_id, selection);

        if (req.access_mode != BufferAccessMode::READ) {
            auto& mask = active_write_masks[req.buffer_id];
            if (_check_and_apply_selection_to_mask(selection, mask.data(), true)) return false; 
        }

        if (p_needs_gpu && rd) {
            if (!buffer_gpu_rids[idx].is_valid()) {
                buffer_gpu_rids[idx] = rd->storage_buffer_create(buf.capacity_bytes);
                buffer_dirty_flags[idx] |= 0x1; 
            }
            if (buffer_dirty_flags[idx] & 0x1) {
                _sync_buffer_to_vram(idx);
            }

            godot::Ref<godot::RDUniform> u; u.instantiate();
            u->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            u->set_binding(static_cast<int>(i));
            u->add_id(buffer_gpu_rids[idx]);
            uniforms.push_back(u);
        }

        r_parts[i] = req; 
        r_parts[i].selection = selection;
        r_parts[i].buffer_version_at_issue = buf.version;
        
        uint8_t* buffer_start = buf.master_block_ptr + buf.memory_offset;
        
        switch (buf.layout_type) {
            case BufferLayoutType::AOS:
            case BufferLayoutType::TILED_SOA:
                r_parts[i].raw_base_ptr = buffer_start;
                r_parts[i].element_stride = buf.element_stride; 
                break;
            case BufferLayoutType::SOA:
            case BufferLayoutType::SPARSE_SET:
                for (uint32_t c = 0; c < buf.column_count; ++c) {
                    if (buf.columns[c].id == req.column_id) {
                        r_parts[i].raw_base_ptr = buffer_start + buf.columns[c].offset;
                        r_parts[i].element_stride = buf.columns[c].type_size;
                        break;
                    }
                }
                break;
            case BufferLayoutType::PAGED:
                r_parts[i].raw_base_ptr = reinterpret_cast<uint8_t*>(buf.extra.paged.table_ptr);
                break;
            case BufferLayoutType::RING:
                r_parts[i].raw_base_ptr = buffer_start + buf.extra.ring.tail_offset;
                break;
            case BufferLayoutType::FLAT:
                r_parts[i].raw_base_ptr = buffer_start;
                r_parts[i].element_stride = buf.element_stride;
                break;
        }
        r_part_count++;
    }

    if (rd && !uniforms.is_empty()) {
        godot::RID rid = rd->uniform_set_create(uniforms, godot::RID(), 0);
        r_uniform_handle = rid.get_id(); 
    }

    return true;
}

void MemoryManagerDOD::_release_grant_core(GrantPartPOD* p_parts, uint32_t p_part_count, uint64_t& r_uniform_handle) {
    std::shared_lock<std::shared_mutex> lock(manager_rw_lock);
    
    if (rd && r_uniform_handle != 0) {
        godot::RID rid;
        *(reinterpret_cast<uint64_t*>(&rid)) = r_uniform_handle;
        if (rid.is_valid()) rd->free_rid(rid);
        r_uniform_handle = 0;
    }

    for (uint32_t i = 0; i < p_part_count; ++i) {
        GrantPartPOD& part = p_parts[i];
        if (part.access_mode != BufferAccessMode::READ) {
            uint32_t idx = id_to_index[part.buffer_id];
            
            GpuSyncCommand cmd { part.buffer_id, 0, buffers[idx].capacity_bytes };
            
            // Backpressure Handling: If the ring saturates, trigger a JIT flush before retrying.
            if (!ring_push(system_command_ring_id, &cmd, sizeof(GpuSyncCommand))) {
                flush_gpu_updates();
                ring_push(system_command_ring_id, &cmd, sizeof(GpuSyncCommand)); 
            }
            
            buffer_dirty_flags[idx] |= 0x1;

            _check_and_apply_selection_to_mask(part.selection, active_write_masks[part.buffer_id].data(), false);
        }
    }
}

void MemoryManagerDOD::_sync_buffer_to_vram(uint32_t p_index) {
    if (!rd) return;
    MemoryBufferPOD& buf = buffers[p_index];
    godot::RID rid = buffer_gpu_rids[p_index];

    godot::PackedByteArray data;
    data.resize(buf.capacity_bytes);
    std::memcpy(data.ptrw(), buf.master_block_ptr + buf.memory_offset, buf.capacity_bytes);
    
    rd->buffer_update(rid, 0, buf.capacity_bytes, data);
    buffer_dirty_flags[p_index] &= ~0x1; 
}

void MemoryManagerDOD::flush_gpu_updates() {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock); // Exclusive lock ensures safety during a massive frame-end flush.
    if (!rd) return;

    GpuSyncCommand cmd;
    while (ring_pop(system_command_ring_id, &cmd, sizeof(GpuSyncCommand))) {
        if (cmd.buffer_id >= id_to_index.size() || id_to_index[cmd.buffer_id] == 0xFFFFFFFF) continue;
        uint32_t idx = id_to_index[cmd.buffer_id];
        if (buffer_dirty_flags[idx] & 0x1) {
            _sync_buffer_to_vram(idx);
        }
    }
}

void MemoryManagerDOD::rebase_master_block(size_t p_new_capacity) {
    std::unique_lock<std::shared_mutex> lock(manager_rw_lock);

#if defined(_MSC_VER)
    uint8_t* new_block = static_cast<uint8_t*>(_aligned_malloc(p_new_capacity, CACHE_LINE_SIZE));
#else
    uint8_t* new_block = static_cast<uint8_t*>(std::aligned_alloc(CACHE_LINE_SIZE, p_new_capacity));
#endif

    std::memcpy(new_block, master_block_ptr, master_used);
    
#if defined(_MSC_VER)
    _aligned_free(master_block_ptr);
#else
    std::free(master_block_ptr);
#endif

    master_block_ptr = new_block;
    master_capacity = p_new_capacity;
    global_version++;

    for (auto& buf : buffers) {
        buf.master_block_ptr = master_block_ptr;
        buf.version++; 
        
        if (buf.layout_type == BufferLayoutType::PAGED) {
            uint8_t* buffer_start = buf.master_block_ptr + buf.memory_offset;
            for (uint32_t i = 0; i < buf.extra.paged.page_count; ++i) {
                buf.extra.paged.table_ptr[i] = buffer_start + (i * buf.extra.paged.page_size_bytes);
            }
        }
        
        uint32_t idx = id_to_index[buf.buffer_id];
        buffer_dirty_flags[idx] |= 0x1; 
    }
}

MemoryBufferPOD* MemoryManagerDOD::get_buffer(uint32_t p_id) {
    // Vector capacity is reserved upfront, so it's safer to return pointers, 
    // but the consuming architecture should ideally copy the POD or hold an ID.
    if (p_id >= id_to_index.size() || id_to_index[p_id] == 0xFFFFFFFF) return nullptr;
    return &buffers[id_to_index[p_id]];
}

} // namespace ideam::core