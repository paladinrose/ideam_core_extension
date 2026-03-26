#ifndef IDEAM_CORE_MEMORY_STRUCTURE_OF_ARRAYS_BUFFER_H
#define IDEAM_CORE_MEMORY_STRUCTURE_OF_ARRAYS_BUFFER_H

#include "../memory_buffer.h"
#include <vector>
#include <cstdint>

namespace ideam::core {

/**
 * StructureOfArraysBuffer
 * Organizes data in parallel lanes (columns) for SIMD efficiency.
 * Column access is performed via integer IDs.
 */
class StructureOfArraysBuffer : public MemoryBuffer {
public:
    struct ColumnInfo {
        uint32_t id;
        size_t type_size;
        size_t offset;
    };

private:
    int64_t max_entities = 0;
    int64_t current_count = 0;
    std::vector<ColumnInfo> columns;

    // --- Compaction State ---
    std::vector<int64_t> death_row;
    bool needs_compaction = false;

    void _calculate_layout();

public:
    StructureOfArraysBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode, size_t p_alignment = 64);
    virtual ~StructureOfArraysBuffer() override = default;

    /**
     * configure
     * Defines the SoA schema. p_layout contains the byte-size of each column's type.
     * The index in p_layout becomes the Column ID.
     */
    void configure(int64_t p_max_entities, const std::vector<size_t>& p_layout);

    // --- Entity Lifecycle ---
    int64_t create_entity();
    void queue_destruction(int64_t p_index);
    void release_destroyed();
    
    virtual void clear() override;

    // --- Metadata & Access ---
    [[nodiscard]] inline int64_t get_count() const { return current_count; }
    [[nodiscard]] inline int64_t get_max_entities() const { return max_entities; }
    
    [[nodiscard]] const ColumnInfo* get_column_info(uint32_t p_column_id) const;
    [[nodiscard]] uint8_t* get_column_ptr(uint32_t p_column_id) const;

    [[nodiscard]] inline bool has_pending_destructions() const { return needs_compaction; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_STRUCTURE_OF_ARRAYS_BUFFER_H