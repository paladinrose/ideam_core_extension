#ifndef IDEAM_CORE_MEMORY_SPARSE_SET_BUFFER_H
#define IDEAM_CORE_MEMORY_SPARSE_SET_BUFFER_H

#include "../memory_buffer.h"
#include "../memory_common.h"
#include <vector>
#include <string>
#include <map>

namespace ideam::core {

/**
 * SparseSetBuffer
 * Port of EntityComponentBuffer. Manages memory for multiple component pools.
 * Uses a Sparse Set (Sparse, Dense, and Data arrays) to allow O(1) lookup and removal
 * while keeping iteration contiguous for cache efficiency.
 */
class SparseSetBuffer : public MemoryBuffer {
public:
    using EntityID = int32_t;
    static constexpr EntityID INVALID_ENTITY = -1;

    enum class SparseSetError {
        NONE,
        OUT_OF_MEMORY,
        POOL_NOT_FOUND,
        ENTITY_NOT_FOUND,
        ENTITY_ALREADY_HAS_COMPONENT
    };

    struct ComponentPool {
        std::string name;
        DataType type;
        size_t data_size;
        size_t alignment;
        
        size_t sparse_offset;      // EntityID -> DenseIndex
        size_t dense_array_offset; // DenseIndex -> EntityID
        size_t data_array_offset;  // DenseIndex -> Component Value
        
        int32_t size = 0;
        int32_t capacity = 0;
    };

private:
    int64_t max_entities = 0;
    std::vector<ComponentPool> pools;
    std::map<std::string, size_t> pool_name_to_idx;

    void _calculate_layout();

public:
    SparseSetBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode = BufferAlignmentMode::STD430) 
        : MemoryBuffer(p_buffer_id, p_mode) {}

    virtual ~SparseSetBuffer() override = default;

    SparseSetBuffer(const SparseSetBuffer&) = delete;
    SparseSetBuffer& operator=(const SparseSetBuffer&) = delete;

    // --- Life Cycle ---
    void initialize_layout(int64_t p_max_entities, const std::vector<std::pair<std::string, DataType>>& p_components);
    
    virtual void rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) override;

    // --- Mutation ---
    bool add_component(EntityID p_entity, const std::string& p_component);
    bool remove_component(EntityID p_entity, const std::string& p_component);
    bool has_component(EntityID p_entity, const std::string& p_component) const;

    // --- Metadata Access ---
    [[nodiscard]] inline int64_t get_max_entities() const { return max_entities; }
    [[nodiscard]] const std::vector<ComponentPool>& get_pools() const { return pools; }
    [[nodiscard]] int64_t get_element_count() const { return max_entities; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_SPARSE_SET_BUFFER_H