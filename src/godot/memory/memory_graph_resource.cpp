#include "memory_graph_resource.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void MemoryGraphResource::_bind_methods() {
    // Inherits all properties and undo/redo logic from IdeamGraphResource
}

std::shared_ptr<core::MemoryGraphDOD> MemoryGraphResource::compile_to_memory_graph(
    core::MemoryManagerDOD* p_manager, 
    std::unordered_map<godot::String, core::NodeID>& r_ui_to_dod_map) const 
{
    // 1. Instantiate the specialized MemoryGraphDOD
    auto memory_graph = std::make_shared<core::MemoryGraphDOD>(p_manager);
    
    godot::TypedArray<godot::Dictionary> current_nodes = get_nodes();
    godot::TypedArray<godot::Dictionary> current_edges = get_edges();

    memory_graph->reserve(current_nodes.size(), current_edges.size());
    r_ui_to_dod_map.clear();
    r_ui_to_dod_map.reserve(current_nodes.size());

    // 2. Compile Nodes
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Dictionary n = current_nodes[i];
        if (!n.has("name") || !n.has("type_id")) continue;

        core::NodeID core_id = memory_graph->add_node(static_cast<uint32_t>(n["type_id"]));
        r_ui_to_dod_map[n["name"]] = core_id;
    }

    // 3. Compile Edges
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        auto from_it = r_ui_to_dod_map.find(e["from"]);
        auto to_it = r_ui_to_dod_map.find(e["to"]);

        if (from_it != r_ui_to_dod_map.end() && to_it != r_ui_to_dod_map.end()) {
            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(e["from_port"]) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(e["to_port"]) : 0;
            memory_graph->connect_nodes(from_it->second, from_port, to_it->second, to_port);
        }
    }

    memory_graph->defragment();
    return memory_graph;
}

} // namespace ideam::godot_ext