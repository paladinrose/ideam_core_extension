#ifndef IDEAM_CORE_SIMULATIONS_FIELD_BUFFER_H
#define IDEAM_CORE_SIMULATIONS_FIELD_BUFFER_H

#include "../simulation_buffer.h"
#include "../../memory/buffers/array_of_structures_buffer.h"
#include <vector>

namespace ideam::core {

/**
 * FieldBuffer
 * High-performance AoS container for simulation fields (e.g., heightmaps, pressure grids).
 * Manages structural layout purely via indices and raw types.
 */
class FieldBuffer : public SimulationBuffer<ArrayOfStructuresBuffer> {
public:
    struct FieldProperty {
        DataType type;
        uint32_t member_id;
    };

private:
    // Cached raw pointer to the internal buffer for direct access
    ArrayOfStructuresBuffer* internal_buffer = nullptr;
    std::vector<FieldProperty> property_registry;

public:
    FieldBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode = BufferAlignmentMode::STD430);
    virtual ~FieldBuffer() override = default;

    /**
     * configure
     * Defines the structure of the "cell" and the size of the field.
     * The index in p_layout defines the Property ID.
     */
    void configure(int64_t p_cell_count, const std::vector<DataType>& p_layout);

    // --- Accessors ---
    [[nodiscard]] inline int64_t get_cell_count() const;
    [[nodiscard]] inline size_t get_cell_stride() const;
    
    [[nodiscard]] uint8_t* get_cell_ptr(int64_t p_cell_index) const;
    [[nodiscard]] const ArrayOfStructuresBuffer::MemberInfo* get_member_info(uint32_t p_property_id) const;

    [[nodiscard]] const std::vector<FieldProperty>& get_property_registry() const { return property_registry; }

    void clear();
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATIONS_FIELD_BUFFER_H