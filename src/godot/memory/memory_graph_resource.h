#ifndef IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H
#define IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H

#include "../graphs/ideam_graph_resource.h"
#include "../../core/memory/memory_graph_dod.h"

namespace ideam::godot_ext {

class MemoryGraphResource : public IdeamGraphResource {
    GDCLASS(MemoryGraphResource, IdeamGraphResource)

protected:
    static void _bind_methods();

public:
    MemoryGraphResource() = default;
    virtual ~MemoryGraphResource() = default;

    /**
     * @brief Compiles the Godot strings/dictionaries into a tightly packed MemoryGraphDOD.
     */
    std::shared_ptr<core::MemoryGraphDOD> compile_to_memory_graph(
        core::MemoryManagerDOD* p_manager, 
        std::unordered_map<godot::String, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext

#endif // IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H