#include "mass_entity_buffer.h"
#include "../../memory/memory_common.h"
#include <algorithm>

namespace ideam::core {

MassEntityBuffer::MassEntityBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode)
    : SimulationBuffer<StructureOfArraysBuffer>(p_buffer_id, SimulationType::MASS, p_mode) {
    
    // Cache the internal pointer from the base template during construction
    internal_buffer = get_internal();
}

void MassEntityBuffer::configure(int64_t p_max_entities, const std::vector<DataType>& p_layout) {
    column_registry.clear();
    
    std::vector<size_t> type_sizes;
    type_sizes.reserve(p_layout.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(p_layout.size()); ++i) {
        DataType type = p_layout[i];
        size_t size = MemoryUtilities::get_type_byte_size(type, get_memory_buffer()->get_alignment_mode());
        
        type_sizes.push_back(size);
        column_registry.push_back({ type, i });
    }

    internal_buffer->configure(p_max_entities, type_sizes);
}

int64_t MassEntityBuffer::create_entity() {
    return internal_buffer->create_entity();
}

std::vector<int64_t> MassEntityBuffer::create_entities_batched(int64_t p_count) {
    int64_t max_entities = internal_buffer->get_max_entities();
    int64_t current_count = internal_buffer->get_count();
    
    int64_t available = max_entities - current_count;
    int64_t to_create = std::min(p_count, available);

    std::vector<int64_t> indices;
    if (to_create <= 0) return indices;

    indices.reserve(to_create);
    for (int64_t i = 0; i < to_create; ++i) {
        int64_t idx = internal_buffer->create_entity();
        if (idx != -1) {
            indices.push_back(idx);
        }
    }

    return indices;
}

void MassEntityBuffer::queue_destruction(int64_t p_index) {
    internal_buffer->queue_destruction(p_index);
}

void MassEntityBuffer::release_destroyed() {
    internal_buffer->release_destroyed();
}

void MassEntityBuffer::clear() {
    internal_buffer->clear();
}

int64_t MassEntityBuffer::get_count() const {
    return internal_buffer->get_count();
}

int64_t MassEntityBuffer::get_max_entities() const {
    return internal_buffer->get_max_entities();
}

uint8_t* MassEntityBuffer::get_column_ptr(uint32_t p_column_id) const {
    return internal_buffer->get_column_ptr(p_column_id);
}

} // namespace ideam::core