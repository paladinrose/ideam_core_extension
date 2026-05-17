#include "task_graph_host.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void TaskGraphHost::_bind_methods() {
    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("setup"), &TaskGraphHost::setup);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_graph", "delta"), &TaskGraphHost::execute_graph);
    godot::ClassDB::bind_method(godot::D_METHOD("is_ready"), &TaskGraphHost::is_ready);

    // Property Setters / Getters
    godot::ClassDB::bind_method(godot::D_METHOD("set_graph_resource", "resource"), &TaskGraphHost::set_graph_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("get_graph_resource"), &TaskGraphHost::get_graph_resource);
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_is_shared", "is_shared"), &TaskGraphHost::set_is_shared);
    godot::ClassDB::bind_method(godot::D_METHOD("get_is_shared"), &TaskGraphHost::get_is_shared);
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_shared_host_path", "path"), &TaskGraphHost::set_shared_host_path);
    godot::ClassDB::bind_method(godot::D_METHOD("get_shared_host_path"), &TaskGraphHost::get_shared_host_path);

    // Inspector Properties
    godot::ClassDB::add_property("TaskGraphHost", godot::PropertyInfo(godot::Variant::OBJECT, "graph_resource", godot::PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"), "set_graph_resource", "get_graph_resource");
    godot::ClassDB::add_property("TaskGraphHost", godot::PropertyInfo(godot::Variant::BOOL, "is_shared"), "set_is_shared", "get_is_shared");
    godot::ClassDB::add_property("TaskGraphHost", godot::PropertyInfo(godot::Variant::NODE_PATH, "shared_host_path", godot::PROPERTY_HINT_NODE_PATH_VALID_TYPES, "TaskGraphHost"), "set_shared_host_path", "get_shared_host_path");
}

void TaskGraphHost::set_graph_resource(const godot::Ref<TaskGraphResource>& p_resource) {
    graph_resource = p_resource;
}

godot::Ref<TaskGraphResource> TaskGraphHost::get_graph_resource() const {
    return graph_resource;
}

void TaskGraphHost::set_is_shared(bool p_shared) {
    is_shared = p_shared;
    notify_property_list_changed(); // Encourages Godot editor to refresh inspector visuals
}

bool TaskGraphHost::get_is_shared() const {
    return is_shared;
}

void TaskGraphHost::set_shared_host_path(const godot::NodePath& p_path) {
    shared_host_path = p_path;
}

godot::NodePath TaskGraphHost::get_shared_host_path() const {
    return shared_host_path;
}

bool TaskGraphHost::is_ready() const {
    // Strict pointer validation. 
    return active_manager != nullptr && active_graph != nullptr;
}

void TaskGraphHost::setup() {
    ERR_FAIL_COND_MSG(graph_resource.is_null(), "TaskGraphHost: Cannot setup. Graph resource is not assigned.");

    if (is_shared) {
        _setup_shared();
    } else {
        _setup_isolated();
    }
}

void TaskGraphHost::_setup_isolated() {
    // Directly extract the target manager from the blueprint logic
    godot::Ref<MemoryManagerResource> manager = graph_resource->get_memory_manager();
    ERR_FAIL_COND_MSG(manager.is_null(), "TaskGraphHost: Isolated setup failed. The provided TaskGraphResource does not contain a valid MemoryManagerResource.");

    // 1. The Declarative Handshake
    // Re-inject the manager to force `update_managed_profiles()`, injecting all 
    // Command Arenas and SoA footprints into the Manager's mathematical sizing logic BEFORE allocation.
    graph_resource->set_memory_manager(manager);

    // 2. Physical Allocation
    if (!manager->is_initialized()) {
        manager->initialize_backend();
    }

    // 3. Backend Fetch & Verification
    active_manager = manager->get_backend();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: MemoryManagerResource failed to provide a valid backend.");

    // 4. Graph Compilation & Defragmentation
    // Because the Manager was just perfectly sized, compiling the graph is now 100% memory-safe.
    active_graph = graph_resource->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
    
    ERR_FAIL_COND_MSG(active_graph == nullptr, "TaskGraphHost: TaskGraphResource failed to compile a valid graph.");
}

void TaskGraphHost::_setup_shared() {
    ERR_FAIL_COND_MSG(shared_host_path.is_empty(), "TaskGraphHost: Shared setup failed. 'is_shared' is true, but no shared_host_path is assigned.");
    
    // Resolve the NodePath to the target host instance in the SceneTree
    godot::Node* target_node = get_node_or_null(shared_host_path);
    TaskGraphHost* target_host = godot::Object::cast_to<TaskGraphHost>(target_node);
    
    ERR_FAIL_NULL_MSG(target_host, "TaskGraphHost: Shared setup failed. Target Host path is invalid or the node is not a TaskGraphHost.");
    
    active_manager = target_host->get_active_manager();
    ERR_FAIL_COND_MSG(active_manager == nullptr, "TaskGraphHost: Target Host does not have an active Memory Manager to share. Ensure the Target Host is setup first.");

    // Note: In a shared environment, the target host must have already run its setup 
    // and successfully allocated a memory footprint large enough to house this node's tasks.
    active_graph = graph_resource->compile_to_task_graph(active_manager.get(), ui_to_dod_map);
    ERR_FAIL_COND_MSG(active_graph == nullptr, "TaskGraphHost: TaskGraphResource failed to compile a valid graph.");
}

void TaskGraphHost::execute_graph(double p_delta) {
    // Hardware-Safe Execution Guard
    if (!is_ready()) {
        return; 
    }

    active_graph->execute_graph_dod(p_delta);
}

} // namespace ideam::godot_ext