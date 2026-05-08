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
    // The setup-time blueprint for our cache-aligned MemoryGrantPOD
    godot::Ref<MemoryGrantResource> memory_grant;

protected:
    static void _bind_methods();

public:
    MemoryGraphNodeResource();
    ~MemoryGraphNodeResource() override = default;

    // --- Configuration ---
    void set_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant);
    godot::Ref<MemoryGrantResource> get_memory_grant() const;

    // --- DOD Compilation Interface ---
    
    /**
     * @brief Ensures both the base topological data AND the specific hardware 
     * memory constraints are valid before allowing the DOD Graph Compiler 
     * to instantiate the raw runtime structures.
     */
    bool validate_for_compilation() const override;
};

} // namespace ideam::godot_ext