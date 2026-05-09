#pragma once

#include "../graphs/ideam_graph_node_resource.h"
#include "memory_grant_resource.h"
#include <godot_cpp/classes/ref.hpp>

namespace ideam::godot_ext {

/**
 * @class MemoryGraphNodeResource
 * @brief The intermediate Authoring component defining a node's memory footprint.
 * * Extends the visual/topological base node to include strict memory access 
 * requirements (Memory Grants). During graph compilation, the MemoryGraphDOD 
 * parses these resources to allocate flat, contiguous structures for the hot-loop.
 */
class MemoryGraphNodeResource : public IdeamGraphNodeResource {
    GDCLASS(MemoryGraphNodeResource, IdeamGraphNodeResource)

private:
    godot::Ref<MemoryGrantResource> memory_grant;
    
    // Abstracted away from the loosely-packed dictionary, mapping 1:1 with DOD architecture
    uint32_t type_id = 0; 

protected:
    static void _bind_methods();

public:
    MemoryGraphNodeResource();
    ~MemoryGraphNodeResource() override = default;

    // --- Configuration ---
    void set_type_id(uint32_t p_id);
    uint32_t get_type_id() const;

    void set_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant);
    godot::Ref<MemoryGrantResource> get_memory_grant() const;

    // --- DOD Compilation Interface ---
    bool validate_for_compilation() const override;
};

} // namespace ideam::godot_ext