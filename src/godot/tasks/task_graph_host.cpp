#include "task_graph_host.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void TaskGraphHost::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("setup_isolated", "manager_resource", "graph_resource"), &TaskGraphHost::setup_isolated);
    godot::ClassDB::bind_method(godot::D_METHOD("setup_shared", "target_host", "graph_resource"), &TaskGraphHost::setup_shared);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_graph", "delta"), &TaskGraphHost::execute_graph);
    godot::ClassDB::bind_method(godot::D_METHOD("is_ready"), &TaskGraphHost::is_ready);
}

void TaskGraphHost::setup_isolated(const godot::Ref<MemoryManagerResource>& p_manager_res, const godot::Ref<TaskGraphResource>& p_graph_res) {
    ERR_FAIL_COND_MSG(p_manager_res.is_null(), "TaskGraphHost: MemoryManagerResource is null.");
    ERR_FAIL_COND_MSG(p_graph_res.is_null(), "TaskGraphHost: TaskGraphResource is null.");

    // Instantiate a new isolated Memory Manager
    // Note: If MemoryManagerResource gains a compile() method, call it here.
    // For now, we instantiate a fresh instance.
    active_manager = std::make_shared<core::MemoryManagerDOD>();

    // Compile the Graph against this isolated manager
    active_graph = p_graph_res->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
}

void TaskGraphHost::setup_shared(TaskGraphHost* p_target_host, const godot::Ref<TaskGraphResource>& p_graph_res) {
    ERR_FAIL_NULL_MSG(p_target_host, "TaskGraphHost: Shared setup failed. Target Host is null.");
    ERR_FAIL_COND_MSG(p_graph_res.is_null(), "TaskGraphHost: TaskGraphResource is null.");
    
    active_manager = p_target_host->get_active_manager();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: Target Host does not have an active Memory Manager to share.");

    // Compile our Graph, but point it at the TARGET's Memory Manager!
    active_graph = p_graph_res->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
}

void TaskGraphHost::execute_graph(double p_delta) {
    if (active_graph) {
        active_graph->execute_graph_dod(p_delta);
    }
}

} // namespace ideam::godot_ext