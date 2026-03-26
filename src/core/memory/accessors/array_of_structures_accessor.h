#ifndef IDEAM_CORE_MEMORY_ARRAY_OF_STRUCTURES_ACCESSOR_H
#define IDEAM_CORE_MEMORY_ARRAY_OF_STRUCTURES_ACCESSOR_H

#include "../memory_buffer_accessor.h"
#include "../buffers/array_of_structures_buffer.h"

namespace ideam::core {

/**
 * ArrayOfStructuresAccessor<T>
 * Specialized for accessing a specific member 'T' within an AoS layout.
 * The stride here is the total size of the "Structure" (element_stride), 
 * not the size of type T.
 */
template<typename T>
class ArrayOfStructuresAccessor : public MemoryBufferAccessor<T> {
private:
    uint32_t target_member_id = 0;
    size_t element_stride = 0;
    size_t member_offset = 0;

public:
    ArrayOfStructuresAccessor(uint32_t p_member_id) 
        : target_member_id(p_member_id) {}

    virtual ~ArrayOfStructuresAccessor() override = default;

    /**
     * resolve_all
     * Bakes the access paths. In AoS, p_buffer_base is the start of the 
     * ENTIRE buffer block. We use the member_offset to find the first T.
     */
    void resolve_all(uint8_t* p_buffer_base, const MemoryBufferSelection* p_selection) override {
        this->source_selection = p_selection;
        this->baked_base_ptr = p_buffer_base;
        this->baked_selection_version = p_selection ? p_selection->version : 0;

        // The head_ptr for an AoS member is: BufferBase + MemberOffset
        this->head_ptr = reinterpret_cast<T*>(p_buffer_base + member_offset);

        if (!p_selection) {
            this->is_contiguous = false;
            this->is_dense_masked = false;
            this->resolved_pointers.clear();
            return;
        }

        // Mode Detection
        if (p_selection->mode == SelectionMode::DENSE) {
            // Even if dense, AoS is only "contiguous" if the element_stride == sizeof(T).
            // Usually, AoS is not contiguous for a single member accessor.
            this->is_contiguous = (element_stride == sizeof(T));
            this->is_dense_masked = true;
            this->resolved_pointers.clear();
        } else {
            this->is_contiguous = false;
            this->is_dense_masked = false;

            const auto& indices = p_selection->indices;
            this->resolved_pointers.clear();
            this->resolved_pointers.reserve(indices.size());

            for (size_t i = 0; i < indices.size(); ++i) {
                // Address = ColumnBase + (EntityIndex * ElementStride)
                // Note: head_ptr already includes the member_offset.
                uint8_t* element_base = reinterpret_cast<uint8_t*>(this->head_ptr);
                this->resolved_pointers.push_back(reinterpret_cast<T*>(element_base + (indices[i] * element_stride)));
            }
        }
    }

    /**
     * bind_member_metadata
     * Called by the Grant or SimulationBuffer to initialize the stride 
     * and offset constants for this specific accessor instance.
     */
    void bind_member_metadata(size_t p_stride, size_t p_offset) {
        element_stride = p_stride;
        member_offset = p_offset;
    }

    /**
     * operator[] Override
     * We must override the base indexer because the stride is not sizeof(T).
     */
    [[nodiscard]] inline T& operator[](int64_t p_index) override {
        if (this->is_dense_masked) {
            uint8_t* base = reinterpret_cast<uint8_t*>(this->head_ptr);
            return *reinterpret_cast<T*>(base + (p_index * element_stride));
        }
        return *(this->resolved_pointers[p_index]);
    }

    [[nodiscard]] inline const T& operator[](int64_t p_index) const override {
        if (this->is_dense_masked) {
            const uint8_t* base = reinterpret_cast<const uint8_t*>(this->head_ptr);
            return *reinterpret_cast<const T*>(base + (p_index * element_stride));
        }
        return *(this->resolved_pointers[p_index]);
    }

    [[nodiscard]] uint32_t get_target_member_id() const { return target_member_id; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_ARRAY_OF_STRUCTURES_ACCESSOR_H