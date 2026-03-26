#ifndef IDEAM_CORE_SIMULATIONS_MASS_ENTITY_BUFFER_H
#define IDEAM_CORE_SIMULATIONS_MASS_ENTITY_BUFFER_H

#include "../simulation_buffer.h"
#include "../../memory/buffers/structure_of_arrays_buffer.h"
#include <vector>

namespace ideam::core {

/**
 * MassEntityBuffer
 * High-performance SoA container for dense entity simulation.
 */
class MassEntityBuffer : public SimulationBuffer<StructureOfArraysBuffer> {
public:
    struct ColumnMapping {
        DataType type;
        uint32_t column_id;
    };

private:
    // Cached raw pointer to the internal buffer for direct access
    StructureOfArraysBuffer* internal_buffer = nullptr;
    std::vector<ColumnMapping> column_registry;

public:
    MassEntityBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode = BufferAlignmentMode::STD430);
    virtual ~MassEntityBuffer() override = default;

    /**
     * configure
     * Sets up the SoA schema using a list of raw types.
     */
    void configure(int64_t p_max_entities, const std::vector<DataType>& p_layout);

    // --- Entity Operations ---
    int64_t create_entity();
    std::vector<int64_t> create_entities_batched(int64_t p_count);
    
    void queue_destruction(int64_t p_index);
    void release_destroyed();
    void clear();

    // --- Metadata & Raw Access ---
    [[nodiscard]] int64_t get_count() const;
    [[nodiscard]] int64_t get_max_entities() const;
    [[nodiscard]] uint8_t* get_column_ptr(uint32_t p_column_id) const;
    
    [[nodiscard]] const std::vector<ColumnMapping>& get_column_registry() const { return column_registry; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATIONS_MASS_ENTITY_BUFFER_H