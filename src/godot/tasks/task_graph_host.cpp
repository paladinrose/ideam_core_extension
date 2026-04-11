#include "task_graph_host.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void TaskGraphHost::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("setup_isolated", "manager_resource", "graph_resource"), &TaskGraphHost::setup_isolated);
    godot::ClassDB::bind_method(godot::D_METHOD("setup_shared", "target_host", "graph_resource"), &TaskGraphHost::setup_shared);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_graph", "delta"), &TaskGraphHost::execute_graph);
    godot::ClassDB::bind_method(godot::D_METHOD("is_ready"), &TaskGraphHost::is_ready);
}

bool TaskGraphHost::is_ready() const {
    // Strict pointer validation. 
    return active_manager != nullptr && active_graph != nullptr;
}

void TaskGraphHost::setup_isolated(const godot::Ref<godot::Resource>& p_manager_res, const godot::Ref<godot::Resource>& p_graph_res) {
    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Boundary crossed successfully!");

    godot::Ref<MemoryManagerResource> manager = p_manager_res;
    godot::Ref<TaskGraphResource> graph = p_graph_res;

    ERR_FAIL_COND_MSG(manager.is_null(), "TaskGraphHost: Provided manager is not a valid MemoryManagerResource.");
    ERR_FAIL_COND_MSG(graph.is_null(), "TaskGraphHost: Provided graph is not a valid TaskGraphResource.");

    // 1. The Declarative Handshake (THE CRITICAL FIX)
    // We forcibly link the Graph to the Manager right here. 
    // This triggers `update_managed_profiles()`, injecting all Command Arenas 
    // and SoA footprints into the Manager's mathematical sizing logic BEFORE allocation.
    graph->set_memory_manager(manager);

    godot::UtilityFunctions::print("Pre-Setup Arena Target: ", manager->get_projected_footprint_string());

    // 2. Physical Allocation
    if (!manager->is_initialized()) {
        godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Initializing Backend...");
        manager->initialize_backend();
    }

    // 3. Backend Fetch & Verification
    active_manager = manager->get_backend();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: MemoryManagerResource failed to provide a valid backend.");

    godot::UtilityFunctions::print("Post-Setup Arena Allocation: ", active_manager->get_allocation_report());
    
    // 4. Graph Compilation & Defragmentation
    // Because the Manager was just perfectly sized, compiling the graph is now 100% memory-safe.
    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Compiling Task Graph...");
    active_graph = graph->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
    
    ERR_FAIL_COND_MSG(active_graph == nullptr, "TaskGraphHost: TaskGraphResource failed to compile a valid graph.");
    godot::UtilityFunctions::print("[DOD Tracker] setup_isolated: Graph compilation complete.");
}

void TaskGraphHost::setup_shared(TaskGraphHost* p_target_host, const godot::Ref<TaskGraphResource>& p_graph_res) {
    ERR_FAIL_NULL_MSG(p_target_host, "TaskGraphHost: Shared setup failed. Target Host is null.");
    ERR_FAIL_COND_MSG(p_graph_res.is_null(), "TaskGraphHost: TaskGraphResource is null.");
    
    active_manager = p_target_host->get_active_manager();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: Target Host does not have an active Memory Manager to share.");

    // Note: In a shared environment, the user must have wired the resources together in the 
    // Godot Editor inspector, ensuring the Target Host's initial allocation was large enough.
    active_graph = p_graph_res->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
    ERR_FAIL_COND_MSG(active_graph == nullptr, "TaskGraphHost: TaskGraphResource failed to compile a valid graph.");
}

void TaskGraphHost::execute_graph(double p_delta) {
    // 1. Hardware-Safe Execution Guard
    if (!is_ready()) {
        return; 
    }

    active_graph->validate_grants();
    active_graph->execute_graph_dod(p_delta);
}

} // namespace ideam::godot_ext