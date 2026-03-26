#ifndef IDEAM_CORE_MEMORY_SPARSE_SET_ACCESSOR_H
#define IDEAM_CORE_MEMORY_SPARSE_SET_ACCESSOR_H

#include "../memory_buffer_accessor.h"
#include "../buffers/sparse_set_buffer.h"

namespace ideam::core {

/**
 * SparseSetAccessor<T>
 * Optimized for Sparse Set architecture (Sparse, Dense, and Data arrays).
 * Provides O(1) random access via EntityID and O(1) contiguous iteration via Dense Index.
 */
template<typename T>
class SparseSetAccessor : public MemoryBufferAccessor<T> {
protected:
    uint32_t pool_id = 0;
    
    // Pointers into the three distinct sub-arrays of the pool, baked during resolution.
    int32_t* sparse_ptr = nullptr;             // EntityID -> DenseIndex
    int32_t* dense_ids_ptr = nullptr;          // DenseIndex -> EntityID
    T* data_ptr = nullptr;                     // DenseIndex -> Component Value (T)

    size_t pool_size = 0;
    int64_t max_entities = 0;
    size_t element_stride = 0;

public:
    SparseSetAccessor(uint32_t p_pool_id) : pool_id(p_pool_id) {}
    virtual ~SparseSetAccessor() override = default;

    /**
     * resolve_all
     * Inherited from MemoryBufferAccessor. Bakes the pointers using the source buffer's 
     * layout and the provided selection (MemoryGrant).
     */
    virtual void resolve_all(uint8_t* p_buffer_base, const MemoryBufferSelection* p_selection) override {
        this->baked_base_ptr = p_buffer_base;
        this->source_selection = p_selection;

        // In a real system call, the Grant/Manager would provide the Buffer* to resolve offsets.
        // We cast to the known buffer type to extract pool metadata.
        auto* ss_buffer = static_cast<SparseSetBuffer*>(this->source_selection->buffer_ptr);
        const auto* pool = ss_buffer->get_pool_info(pool_id);

        if (pool) {
            sparse_ptr = reinterpret_cast<int32_t*>(p_buffer_base + pool->sparse_offset);
            dense_ids_ptr = reinterpret_cast<int32_t*>(p_buffer_base + pool->dense_array_offset);
            data_ptr = reinterpret_cast<T*>(p_buffer_base + pool->data_array_offset);
            
            pool_size = pool->size;
            element_stride = pool->data_size;
            max_entities = ss_buffer->get_max_entities();

            // The head_ptr for a SparseSetAccessor is the start of the contiguous data array.
            this->head_ptr = data_ptr;
        }
    }

    // --- Random Access (EntityID) ---

    /**
     * get
     * Retrieves the component for a specific EntityID.
     * Returns nullptr if the entity does not have the component or ID is out of bounds.
     */
    [[nodiscard]] inline T* get(int32_t p_entity) {
        if (p_entity < 0 || p_entity >= max_entities) return nullptr;
        
        int32_t dense_idx = sparse_ptr[p_entity];
        if (dense_idx == -1) return nullptr;

        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(data_ptr) + (dense_idx * element_stride));
    }

    [[nodiscard]] inline const T* get(int32_t p_entity) const {
        if (p_entity < 0 || p_entity >= max_entities) return nullptr;
        
        int32_t dense_idx = sparse_ptr[p_entity];
        if (dense_idx == -1) return nullptr;

        return reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(data_ptr) + (dense_idx * element_stride));
    }

    // --- Contiguous Access (Dense Index) ---

    /**
     * operator[]
     * Accesses the data array contiguously by its dense index [0 ... size-1].
     */
    [[nodiscard]] inline T& operator[](int64_t p_dense_idx) override {
        return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(data_ptr) + (p_dense_idx * element_stride));
    }

    [[nodiscard]] inline const T& operator[](int64_t p_dense_idx) const override {
        return *reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(data_ptr) + (p_dense_idx * element_stride));
    }

    /**
     * get_entity_at
     * Returns the EntityID associated with a specific dense index.
     * Essential for systems that need to know "who" they are processing during a loop.
     */
    [[nodiscard]] inline int32_t get_entity_at(int32_t p_dense_idx) const {
        return dense_ids_ptr[p_dense_idx];
    }

    // --- Metadata ---

    [[nodiscard]] inline size_t size() const { return pool_size; }
    [[nodiscard]] inline uint32_t get_pool_id() const { return pool_id; }
    [[nodiscard]] inline bool has_entity(int32_t p_entity) const {
        return (p_entity >= 0 && p_entity < max_entities && sparse_ptr[p_entity] != -1);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_SPARSE_SET_ACCESSOR_H