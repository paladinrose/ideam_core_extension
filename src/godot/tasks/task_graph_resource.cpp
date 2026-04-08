#include "task_graph_resource.h"
#include "../../core/tasks/native_task_registry.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void TaskGraphResource::_bind_methods() {
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_GODOT_REFLECTION", TASK_GODOT_REFLECTION);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_NATIVE_CPU", TASK_NATIVE_CPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_COMPUTE_GPU", TASK_COMPUTE_GPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_QUERY_CULLER", TASK_QUERY_CULLER);
}

std::shared_ptr<core::TaskGraphDOD> TaskGraphResource::compile_to_task_graph(
    core::MemoryManagerDOD* p_manager, 
    godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const 
{
    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Allocating TaskGraphDOD...");
    auto task_graph = std::make_shared<core::TaskGraphDOD>(p_manager);
    
    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Fetching Nodes/Edges from Resource...");
    godot::TypedArray<godot::Dictionary> current_nodes = get_nodes();
    godot::TypedArray<godot::Dictionary> current_edges = get_edges();

    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Reserving Graph Capacity...");
    task_graph->reserve(current_nodes.size(), current_edges.size());
    r_ui_to_dod_map.clear();

    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Iterating nodes...");
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Dictionary n = current_nodes[i];
        if (!n.has("name") || !n.has("task_type")) continue;

        godot::StringName ui_name = n["name"];
        
        // PROACTIVE FIX: Safe Variant Cast
        int64_t safe_type = n["task_type"]; 
        core::TaskTypeDOD task_type = static_cast<core::TaskTypeDOD>(static_cast<uint32_t>(safe_type));

        core::NodeID core_id = task_graph->add_task_node(task_type);
        r_ui_to_dod_map[ui_name] = core_id;

        if (n.has("properties")) {
            godot::Dictionary props = n["properties"];

            if (props.has("transient_bytes")) {
                int64_t safe_t_bytes = props["transient_bytes"];
                task_graph->set_node_transient_requirement(core_id, static_cast<uint32_t>(safe_t_bytes));
            }

            switch (task_type) {
                case core::TaskTypeDOD::GODOT_REFLECTION: {
                    godot::Object* target = props.has("reflection_target") ? godot::Object::cast_to<godot::Object>(props["reflection_target"]) : nullptr;
                    godot::StringName method = props.has("execution_method") ? static_cast<godot::StringName>(props["execution_method"]) : godot::StringName("");
                    task_graph->configure_cpu_task(core_id, target, method);
                    break;
                }
                case core::TaskTypeDOD::COMPUTE_GPU: {
                    break;
                }
                case core::TaskTypeDOD::NATIVE_CPU:
                case core::TaskTypeDOD::QUERY_CULLER: {
                    if (props.has("native_class")) {
                        godot::StringName native_class = props["native_class"];
                        godot::UtilityFunctions::print("[DOD Tracker] Instantiating Native Task: ", native_class);
                        
                        auto task_instance = core::NativeTaskRegistry::create(native_class);
                        if (task_instance) {
                            task_graph->configure_native_interface(core_id, std::move(task_instance));
                        } else {
                            godot::UtilityFunctions::printerr("TaskGraph Compiler: Unable to find registered native task '", native_class, "' for node '", ui_name, "'");
                        }
                    }
                    break;
                }
            }
        }
    }

    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Compiling edges...");
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        godot::StringName from_name = e["from"];
        godot::StringName to_name = e["to"];

        if (r_ui_to_dod_map.has(from_name) && r_ui_to_dod_map.has(to_name)) {
            core::NodeID from_id = r_ui_to_dod_map[from_name];
            core::NodeID to_id = r_ui_to_dod_map[to_name];

            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(static_cast<int64_t>(e["from_port"])) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(static_cast<int64_t>(e["to_port"])) : 0;
            
            task_graph->connect_nodes(from_id, from_port, to_id, to_port);
        }
    }

    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Defragmenting...");
    task_graph->defragment();
    
    godot::UtilityFunctions::print("[DOD Tracker] compile_to_task_graph: Done!");
    return task_graph;
}

} // namespace ideam::godot_ext