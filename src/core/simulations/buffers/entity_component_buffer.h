#ifndef IDEAM_CORE_SIMULATIONS_ENTITY_COMPONENT_BUFFER_H
#define IDEAM_CORE_SIMULATIONS_ENTITY_COMPONENT_BUFFER_H

#include "../simulation_buffer.h"
#include "../../memory/buffers/sparse_set_buffer.h"
#include <vector>
#include <string>

namespace ideam::core {

/**
 * EntityComponentBuffer
 * Core simulation wrapper for Sparse Set component storage.
 * Manages pools of components associated with Entity IDs.
 */
class EntityComponentBuffer : public SimulationBuffer<SparseSetBuffer> {
public:
    using EntityID = SparseSetBuffer::EntityID;

    struct ComponentRegistryEntry {
        DataType type;
        uint32_t pool_id;
    };

private:
    SparseSetBuffer* internal_buffer = nullptr;
    std::vector<ComponentRegistryEntry> registry;

public:
    EntityComponentBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode = BufferAlignmentMode::STD430);
    virtual ~EntityComponentBuffer() override = default;

    /**
     * configure
     * Sets up the sparse set pools. The index in p_layout defines the Pool ID.
     */
    void configure(int64_t p_max_entities, const std::vector<DataType>& p_layout);

    // --- Component Operations ---
    bool add_component(EntityID p_entity, uint32_t p_pool_id);
    bool remove_component(EntityID p_entity, uint32_t p_pool_id);
    bool has_component(EntityID p_entity, uint32_t p_pool_id) const;

    // --- Accessors ---
    [[nodiscard]] uint8_t* get_component_ptr(EntityID p_entity, uint32_t p_pool_id) const;
    [[nodiscard]] const SparseSetBuffer::ComponentPool* get_pool_info(uint32_t p_pool_id) const;
    
    [[nodiscard]] int64_t get_max_entities() const;
    [[nodiscard]] const std::vector<ComponentRegistryEntry>& get_registry() const { return registry; }

    void clear();
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATIONS_ENTITY_COMPONENT_BUFFER_H