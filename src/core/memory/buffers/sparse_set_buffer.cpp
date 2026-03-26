#include "sparse_set_buffer.h"
#include <algorithm>
#include <cstring>

namespace ideam::core {

void SparseSetBuffer::initialize_layout(int64_t p_max_entities, const std::vector<std::pair<std::string, DataType>>& p_components) {
    max_entities = p_max_entities;
    pools.clear();
    pool_name_to_idx.clear();

    for (const auto& comp : p_components) {
        ComponentPool pool;
        pool.name = comp.first;
        pool.type = comp.second;
        pool.data_size = MemoryUtilities::get_type_byte_size(pool.type, alignment_mode);
        pool.alignment = MemoryUtilities::get_type_alignment(pool.type, alignment_mode);
        pool.capacity = static_cast<int32_t>(p_max_entities);
        pool.size = 0;
        
        pool_name_to_idx[pool.name] = pools.size();
        pools.push_back(pool);
    }

    _calculate_layout();
    bump_version(); 
}

void SparseSetBuffer::_calculate_layout() {
    size_t current_offset = 0;
    const size_t global_alignment = 64; 

    // 1. Sparse Arrays (Maps EntityID to Dense Index)
    // We group all sparse arrays together for better spatial locality during membership checks.
    for (auto& pool : pools) {
        pool.sparse_offset = current_offset;
        size_t sparse_size = max_entities * sizeof(int32_t);
        current_offset = MemoryUtilities::align_to(current_offset + sparse_size, global_alignment);
    }

    // 2. Dense and Data Arrays
    for (auto& pool : pools) {
        // The Dense Array maps Dense Index back to EntityID.
        pool.dense_array_offset = current_offset;
        size_t dense_size = max_entities * sizeof(EntityID);
        current_offset = MemoryUtilities::align_to(current_offset + dense_size, global_alignment);

        // The Data Array stores the actual component values.
        current_offset = MemoryUtilities::align_to(current_offset, pool.alignment);
        pool.data_array_offset = current_offset;
        
        size_t data_region_size = max_entities * pool.data_size;
        current_offset = MemoryUtilities::align_to(current_offset + data_region_size, global_alignment);
    }

    used_bytes = current_offset;
}

void SparseSetBuffer::rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) {
    bool is_brand_new_allocation = (p_new_master != master_block_ptr);
    
    MemoryBuffer::rebase(p_new_master, p_new_offset, p_new_capacity);

    // Critical: Sparse Set relies on sparse slots being -1 to indicate "no component".
    if (is_brand_new_allocation) {
        uint8_t* base = get_raw_ptr();
        for (const auto& pool : pools) {
            int32_t* sparse_ptr = reinterpret_cast<int32_t*>(base + pool.sparse_offset);
            std::fill_n(sparse_ptr, max_entities, -1);
        }
    }
}

bool SparseSetBuffer::add_component(EntityID p_entity, const std::string& p_component) {
    auto it = pool_name_to_idx.find(p_component);
    if (it == pool_name_to_idx.end()) return false;

    ComponentPool& pool = pools[it->second];
    uint8_t* base = get_raw_ptr();
    int32_t* sparse_ptr = reinterpret_cast<int32_t*>(base + pool.sparse_offset);
    
    if (p_entity < 0 || p_entity >= max_entities || sparse_ptr[p_entity] != -1) return false;
    if (pool.size >= pool.capacity) return false;

    int32_t dense_idx = pool.size;
    EntityID* dense_ptr = reinterpret_cast<EntityID*>(base + pool.dense_array_offset);
    
    sparse_ptr[p_entity] = dense_idx;
    dense_ptr[dense_idx] = p_entity;

    // Initialize memory to zero to avoid simulation noise.
    uint8_t* data_ptr = base + pool.data_array_offset + (dense_idx * pool.data_size);
    std::memset(data_ptr, 0, pool.data_size);

    pool.size++;
    return true;
}

bool SparseSetBuffer::remove_component(EntityID p_entity, const std::string& p_component) {
    auto it = pool_name_to_idx.find(p_component);
    if (it == pool_name_to_idx.end()) return false;

    ComponentPool& pool = pools[it->second];
    uint8_t* base = get_raw_ptr();
    int32_t* sparse_ptr = reinterpret_cast<int32_t*>(base + pool.sparse_offset);
    
    if (p_entity < 0 || p_entity >= max_entities) return false;
    int32_t dense_idx_to_remove = sparse_ptr[p_entity];

    if (dense_idx_to_remove == -1) return false;

    EntityID* dense_ptr = reinterpret_cast<EntityID*>(base + pool.dense_array_offset);
    uint8_t* data_base_ptr = base + pool.data_array_offset;

    int32_t last_dense_idx = pool.size - 1;
    EntityID last_entity = dense_ptr[last_dense_idx];

    // Swap and Pop: Move the last element into the removed slot to keep data contiguous.
    if (dense_idx_to_remove != last_dense_idx) {
        dense_ptr[dense_idx_to_remove] = last_entity;
        std::memcpy(data_base_ptr + (dense_idx_to_remove * pool.data_size),
                    data_base_ptr + (last_dense_idx * pool.data_size),
                    pool.data_size);
        
        sparse_ptr[last_entity] = dense_idx_to_remove;
    }

    sparse_ptr[p_entity] = -1;
    pool.size--;
    return true;
}

bool SparseSetBuffer::has_component(EntityID p_entity, const std::string& p_component) const {
    auto it = pool_name_to_idx.find(p_component);
    if (it == pool_name_to_idx.end()) return false;
    if (p_entity < 0 || p_entity >= max_entities) return false;

    int32_t* sparse_ptr = reinterpret_cast<int32_t*>(get_raw_ptr() + pools[it->second].sparse_offset);
    return sparse_ptr[p_entity] != -1;
}

} // namespace ideam::core