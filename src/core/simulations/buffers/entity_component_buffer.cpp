#include "entity_component_buffer.h"
#include <string>

namespace ideam::core {

EntityComponentBuffer::EntityComponentBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode)
    : SimulationBuffer<SparseSetBuffer>(p_buffer_id, SimulationType::ENTITY_COMPONENT, p_mode) {
    
    internal_buffer = get_internal();
}

void EntityComponentBuffer::configure(int64_t p_max_entities, const std::vector<DataType>& p_layout) {
    registry.clear();
    
    // SparseSetBuffer expects named pairs for layout. 
    // We use index-based strings to satisfy the memory buffer's internal map
    // while keeping the core simulation path numerical.
    std::vector<std::pair<std::string, DataType>> internal_layout;
    internal_layout.reserve(p_layout.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(p_layout.size()); ++i) {
        DataType type = p_layout[i];
        internal_layout.push_back({ std::to_string(i), type });
        registry.push_back({ type, i });
    }

    internal_buffer->initialize_layout(p_max_entities, internal_layout);
}

bool EntityComponentBuffer::add_component(EntityID p_entity, uint32_t p_pool_id) {
    return internal_buffer->add_component(p_entity, std::to_string(p_pool_id));
}

bool EntityComponentBuffer::remove_component(EntityID p_entity, uint32_t p_pool_id) {
    return internal_buffer->remove_component(p_entity, std::to_string(p_pool_id));
}

bool EntityComponentBuffer::has_component(EntityID p_entity, uint32_t p_pool_id) const {
    return internal_buffer->has_component(p_entity, std::to_string(p_pool_id));
}

uint8_t* EntityComponentBuffer::get_component_ptr(EntityID p_entity, uint32_t p_pool_id) const {
    if (p_pool_id >= registry.size()) return nullptr;

    const auto& pools = internal_buffer->get_pools();
    const auto& pool = pools[p_pool_id];
    
    uint8_t* base = internal_buffer->get_raw_ptr();
    int32_t* sparse_ptr = reinterpret_cast<int32_t*>(base + pool.sparse_offset);
    
    if (p_entity < 0 || p_entity >= internal_buffer->get_max_entities()) return nullptr;
    
    int32_t dense_idx = sparse_ptr[p_entity];
    if (dense_idx == -1) return nullptr;

    return base + pool.data_array_offset + (dense_idx * pool.data_size);
}

const SparseSetBuffer::ComponentPool* EntityComponentBuffer::get_pool_info(uint32_t p_pool_id) const {
    const auto& pools = internal_buffer->get_pools();
    if (p_pool_id >= pools.size()) return nullptr;
    return &pools[p_pool_id];
}

int64_t EntityComponentBuffer::get_max_entities() const {
    return internal_buffer->get_max_entities();
}

void EntityComponentBuffer::clear() {
    internal_buffer->clear();
}

} // namespace ideam::core