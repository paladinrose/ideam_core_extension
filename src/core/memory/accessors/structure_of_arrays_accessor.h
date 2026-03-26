#ifndef IDEAM_CORE_MEMORY_STRUCTURE_OF_ARRAYS_ACCESSOR_H
#define IDEAM_CORE_MEMORY_STRUCTURE_OF_ARRAYS_ACCESSOR_H

#include "../memory_buffer_accessor.h"
#include "../buffers/structure_of_arrays_buffer.h"

namespace ideam::core {

/**
 * StructureOfArraysAccessor<T>
 * A specialized accessor for a single lane (column) within a StructureOfArraysBuffer.
 * Unlike AoS, the stride for an SoA accessor is always sizeof(T) because data 
 * within a single lane is contiguous.
 */
template<typename T>
class StructureOfArraysAccessor : public MemoryBufferAccessor<T> {
private:
    uint32_t target_column_id = 0;
    size_t type_stride = sizeof(T);

public:
    StructureOfArraysAccessor(uint32_t p_column_id) 
        : target_column_id(p_column_id) {}

    virtual ~StructureOfArraysAccessor() override = default;

    /**
     * resolve_all
     * Bakes the access paths based on the current buffer state and selection.
     * In SoA, p_buffer_base should be the start of the SPECIFIC COLUMN lane,
     * not the start of the entire MemoryBuffer.
     */
    void resolve_all(uint8_t* p_column_base, const MemoryBufferSelection* p_selection) override {
        this->source_selection = p_selection;
        this->baked_base_ptr = p_column_base;
        this->baked_selection_version = p_selection ? p_selection->version : 0;
        
        // SoA specific: The head pointer is the start of this specific data lane.
        this->head_ptr = reinterpret_cast<T*>(p_column_base);

        if (!p_selection) {
            this->is_contiguous = false;
            this->is_dense_masked = false;
            this->resolved_pointers.clear();
            return;
        }

        if (p_selection->mode == SelectionMode::DENSE) {
            // In DENSE mode, we treat the buffer as a stream. 
            // The user uses the mask to skip inactive elements.
            this->is_contiguous = true; 
            this->is_dense_masked = true;
            this->resolved_pointers.clear();
        } else {
            // In SPARSE mode, we pre-calculate the address for every selected index.
            this->is_contiguous = false;
            this->is_dense_masked = false;
            
            const auto& indices = p_selection->indices;
            this->resolved_pointers.clear();
            this->resolved_pointers.resize(indices.size());
            
            for (size_t i = 0; i < indices.size(); ++i) {
                // Address = ColumnBase + (EntityIndex * sizeof(T))
                this->resolved_pointers[i] = &this->head_ptr[indices[i]];
            }
        }
    }

    [[nodiscard]] uint32_t get_target_column_id() const { return target_column_id; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_STRUCTURE_OF_ARRAYS_ACCESSOR_H