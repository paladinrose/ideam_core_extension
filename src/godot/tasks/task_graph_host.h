#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include "../../core/memory/memory_manager_dod.h"
#include "../../core/tasks/task_graph_dod.h"
#include "../memory/memory_manager_resource.h"
#include "task_graph_resource.h"
#include <memory>

namespace ideam::godot_ext {

class TaskGraphHost : public godot::Node {
    GDCLASS(TaskGraphHost, godot::Node)

protected:
    // --- Inspector Properties ---
    godot::Ref<TaskGraphResource> graph_resource;
    bool is_shared = false;
    godot::NodePath shared_host_path;

    // --- Runtime DOD Backend ---
    std::shared_ptr<core::MemoryManagerDOD> active_manager;
    std::shared_ptr<core::TaskGraphDOD> active_graph;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map;

    static void _bind_methods();

    // Internal routing for the unified setup
    void _setup_isolated();
    void _setup_shared();

public:
    TaskGraphHost() = default;
    virtual ~TaskGraphHost() override = default;

    // --- Property Getters/Setters ---
    void set_graph_resource(const godot::Ref<TaskGraphResource>& p_resource);
    godot::Ref<TaskGraphResource> get_graph_resource() const;

    void set_is_shared(bool p_shared);
    bool get_is_shared() const;

    void set_shared_host_path(const godot::NodePath& p_path);
    godot::NodePath get_shared_host_path() const;

    /**
     * @brief Consolidated setup method. Routes to Isolated or Shared execution 
     * based on Inspector configuration. Requires the node to be in the SceneTree 
     * if operating in Shared mode (to resolve the NodePath).
     */
    void setup();

    /**
     * @brief Ticks the active graph. 
     */
    void execute_graph(double p_delta);

    /**
     * @brief Returns true if this host has mathematically valid memory and a compiled graph.
     */
    bool is_ready() const;

    // --- C++ Internal Accessors ---
    std::shared_ptr<core::MemoryManagerDOD> get_active_manager() const { return active_manager; }
    std::shared_ptr<core::TaskGraphDOD> get_active_graph() const { return active_graph; }
};

} // namespace ideam::godot_ext