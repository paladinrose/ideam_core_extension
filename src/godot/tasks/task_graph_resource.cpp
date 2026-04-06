#include "task_graph_resource.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void TaskGraphResource::_bind_methods() {
    // Inherits all serialization properties from MemoryGraphResource/IdeamGraphResource
    // Expose the Enum to GDScript for UI tools
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_GODOT_REFLECTION", TASK_GODOT_REFLECTION);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_NATIVE_CPU", TASK_NATIVE_CPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_COMPUTE_GPU", TASK_COMPUTE_GPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_QUERY_CULLER", TASK_QUERY_CULLER);
}

std::shared_ptr<core::TaskGraphDOD> TaskGraphResource::compile_to_task_graph(
    core::MemoryManagerDOD* p_manager, 
    std::unordered_map<godot::String, core::NodeID>& r_ui_to_dod_map) const 
{
    auto task_graph = std::make_shared<core::TaskGraphDOD>(p_manager);
    
    godot::TypedArray<godot::Dictionary> current_nodes = get_nodes();
    godot::TypedArray<godot::Dictionary> current_edges = get_edges();

    task_graph->reserve(current_nodes.size(), current_edges.size());
    r_ui_to_dod_map.clear();
    r_ui_to_dod_map.reserve(current_nodes.size());

    // 1. Compile Nodes & Configure Task Data
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Dictionary n = current_nodes[i];
        if (!n.has("name") || !n.has("task_type")) continue;

        uint32_t raw_type = static_cast<uint32_t>(n["task_type"]);
        core::TaskTypeDOD task_type = static_cast<core::TaskTypeDOD>(raw_type);

        // Add node to backend
        core::NodeID core_id = task_graph->add_task_node(task_type);
        r_ui_to_dod_map[n["name"]] = core_id;

        // Apply Configuration from Properties Dictionary
        if (n.has("properties")) {
            godot::Dictionary props = n["properties"];

            // Transient Memory Requirement
            if (props.has("transient_bytes")) {
                uint32_t t_bytes = static_cast<uint32_t>(props["transient_bytes"]);
                task_graph->set_node_transient_requirement(core_id, t_bytes);
            }

            // Route Specific Task Configuration
            switch (task_type) {
                case core::TaskTypeDOD::GODOT_REFLECTION: {
                    godot::Object* target = props.has("reflection_target") ? Object::cast_to<godot::Object>(props["reflection_target"]) : nullptr;
                    godot::StringName method = props.has("execution_method") ? static_cast<godot::StringName>(props["execution_method"]) : godot::StringName("");                    task_graph->configure_cpu_task(core_id, target, method);
                    break;
                }
                case core::TaskTypeDOD::COMPUTE_GPU: {
                    // Placeholder for future GPU configuration extraction
                    // e.g., task_graph->configure_gpu_task(core_id, pipeline_rid, x, y, z);
                    break;
                }
                case core::TaskTypeDOD::NATIVE_CPU:
                case core::TaskTypeDOD::QUERY_CULLER:
                    // Native C++ tasks will be instantiated via a Registry pattern during compilation in the future.
                    break;
            }
            
            // Note: Port mappings and constants would be extracted and compiled here as well
            // using task_graph->set_port_mappings() and set_port_constants()
        }
    }

    // 2. Compile Edges
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        auto from_it = r_ui_to_dod_map.find(e["from"]);
        auto to_it = r_ui_to_dod_map.find(e["to"]);

        if (from_it != r_ui_to_dod_map.end() && to_it != r_ui_to_dod_map.end()) {
            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(e["from_port"]) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(e["to_port"]) : 0;
            task_graph->connect_nodes(from_it->second, from_port, to_it->second, to_port);
        }
    }

    task_graph->defragment();
    return task_graph;
}

} // namespace ideam::godot_ext