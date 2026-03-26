#include "structure_of_arrays_buffer.h"
#include <algorithm>
#include <cstring>

namespace ideam::core {

StructureOfArraysBuffer::StructureOfArraysBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode, size_t p_alignment)
    : MemoryBuffer(p_buffer_id, p_mode) {
    alignment_requirement = p_alignment;
}

void StructureOfArraysBuffer::configure(int64_t p_max_entities, const std::vector<size_t>& p_layout) {
    max_entities = p_max_entities;
    columns.clear();
    columns.reserve(p_layout.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(p_layout.size()); ++i) {
        ColumnInfo info;
        info.id = i;
        info.type_size = p_layout[i];
        info.offset = 0; // Calculated in _calculate_layout
        columns.push_back(info);
    }

    _calculate_layout();
}

void StructureOfArraysBuffer::_calculate_layout() {
    size_t current_offset = 0;

    for (auto& col : columns) {
        // Align the start of every column to the buffer's alignment requirement (e.g., 64 bytes)
        size_t remainder = current_offset % alignment_requirement;
        if (remainder != 0) {
            current_offset += (alignment_requirement - remainder);
        }

        col.offset = current_offset;
        
        // Each column consumes (max_entities * type_size) bytes
        size_t column_bytes = static_cast<size_t>(max_entities) * col.type_size;
        current_offset += column_bytes;
    }

    used_bytes = current_offset;
    bump_version();
}

int64_t StructureOfArraysBuffer::create_entity() {
    if (current_count >= max_entities) {
        return -1;
    }

    uint8_t* base = get_raw_ptr();
    int64_t new_idx = current_count;

    // Zero-initialize the new entity's slots across all lanes
    for (const auto& col : columns) {
        uint8_t* ptr = base + col.offset + (new_idx * col.type_size);
        std::memset(ptr, 0, col.type_size);
    }

    current_count++;
    return new_idx;
}

void StructureOfArraysBuffer::queue_destruction(int64_t p_index) {
    if (p_index < 0 || p_index >= current_count) {
        return;
    }
    death_row.push_back(p_index);
    needs_compaction = true;
}

void StructureOfArraysBuffer::release_destroyed() {
    if (!needs_compaction || death_row.empty()) {
        return;
    }

    // Sort and unique to prepare for compaction
    std::sort(death_row.begin(), death_row.end());
    death_row.erase(std::unique(death_row.begin(), death_row.end()), death_row.end());

    uint8_t* base = get_raw_ptr();
    if (!base) return;

    int64_t write_idx = 0;
    size_t death_ptr = 0;

    // Shift-down compaction across all columns
    for (int64_t read_idx = 0; read_idx < current_count; ++read_idx) {
        if (death_ptr < death_row.size() && read_idx == death_row[death_ptr]) {
            death_ptr++;
            continue;
        }

        if (read_idx != write_idx) {
            for (const auto& col : columns) {
                uint8_t* col_base = base + col.offset;
                std::memcpy(col_base + (write_idx * col.type_size),
                            col_base + (read_idx * col.type_size),
                            col.type_size);
            }
        }
        write_idx++;
    }

    current_count = write_idx;
    death_row.clear();
    needs_compaction = false;
    
    // Compaction changes indices/memory layout, must invalidate accessors
    bump_version();
}

void StructureOfArraysBuffer::clear() {
    current_count = 0;
    death_row.clear();
    needs_compaction = false;
    MemoryBuffer::clear();
}

const StructureOfArraysBuffer::ColumnInfo* StructureOfArraysBuffer::get_column_info(uint32_t p_column_id) const {
    if (p_column_id >= columns.size()) {
        return nullptr;
    }
    return &columns[p_column_id];
}

uint8_t* StructureOfArraysBuffer::get_column_ptr(uint32_t p_column_id) const {
    const ColumnInfo* info = get_column_info(p_column_id);
    if (!info) {
        return nullptr;
    }
    return get_raw_ptr() + info->offset;
}

} // namespace ideam::core