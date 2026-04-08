#include "task_graph_host.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void TaskGraphHost::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("setup_isolated", "manager_resource", "graph_resource"), &TaskGraphHost::setup_isolated);
    godot::ClassDB::bind_method(godot::D_METHOD("setup_shared", "target_host", "graph_resource"), &TaskGraphHost::setup_shared);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_graph", "delta"), &TaskGraphHost::execute_graph);
    godot::ClassDB::bind_method(godot::D_METHOD("is_ready"), &TaskGraphHost::is_ready);
}

void TaskGraphHost::setup_isolated(const godot::Ref<godot::Resource>& p_manager_res, const godot::Ref<godot::Resource>& p_graph_res) {
    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Boundary crossed successfully!");

    // Safely cast the generic resources to our custom GDExtension resources
    godot::Ref<MemoryManagerResource> manager = p_manager_res;
    godot::Ref<TaskGraphResource> graph = p_graph_res;

    ERR_FAIL_COND_MSG(manager.is_null(), "TaskGraphHost: Provided manager is not a valid MemoryManagerResource.");
    ERR_FAIL_COND_MSG(graph.is_null(), "TaskGraphHost: Provided graph is not a valid TaskGraphResource.");

    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Initializing Backend...");

    if (!manager->is_initialized()) {
        manager->initialize_backend();
    }
    
    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Fetching active manager...");
    active_manager = manager->get_backend();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: MemoryManagerResource failed to provide a valid backend.");

    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Compiling Task Graph...");
    active_graph = graph->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
    
    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Graph compilation complete.");
    ERR_FAIL_COND_MSG(active_graph == nullptr, "TaskGraphHost: TaskGraphResource failed to compile a valid graph.");
}

void TaskGraphHost::setup_shared(TaskGraphHost* p_target_host, const godot::Ref<TaskGraphResource>& p_graph_res) {
    ERR_FAIL_NULL_MSG(p_target_host, "TaskGraphHost: Shared setup failed. Target Host is null.");
    ERR_FAIL_COND_MSG(p_graph_res.is_null(), "TaskGraphHost: TaskGraphResource is null.");
    
    active_manager = p_target_host->get_active_manager();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: Target Host does not have an active Memory Manager to share.");

    active_graph = p_graph_res->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
}

void TaskGraphHost::execute_graph(double p_delta) {
    if (active_graph) {
        active_graph->execute_graph_dod(p_delta);
    }
}

} // namespace ideam::godot_ext