#ifndef IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H
#define IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H

#include "../graphs/ideam_graph_resource.h"
#include "../../core/memory/memory_graph_dod.h"
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <memory>

namespace ideam::godot_ext {

class MemoryGraphResource : public IdeamGraphResource {
    GDCLASS(MemoryGraphResource, IdeamGraphResource)

protected:
    static void _bind_methods();

public:
    MemoryGraphResource() = default;
    ~MemoryGraphResource() = default;

    std::shared_ptr<core::MemoryGraphDOD> compile_to_memory_graph(
        core::MemoryManagerDOD* p_manager, 
        godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext

#endif // IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H