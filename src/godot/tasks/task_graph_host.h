#ifndef IDEAM_GODOT_TASK_GRAPH_HOST_H
#define IDEAM_GODOT_TASK_GRAPH_HOST_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../../core/memory/memory_manager_dod.h"
#include "../../core/tasks/task_graph_dod.h"
#include "../memory/memory_manager_resource.h"
#include "task_graph_resource.h"
#include <memory>

namespace ideam::godot_ext {

class TaskGraphHost : public godot::Node {
    GDCLASS(TaskGraphHost, godot::Node)

protected:
    std::shared_ptr<core::MemoryManagerDOD> active_manager;
    std::shared_ptr<core::TaskGraphDOD> active_graph;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map;

    static void _bind_methods();

public:
    TaskGraphHost() = default;
    virtual ~TaskGraphHost() override = default;

    /**
     * @brief Creates a completely isolated DOD execution environment.
     * Orchestrates the strict memory handshake before compiling the Graph.
     */
    void setup_isolated(const godot::Ref<godot::Resource>& p_manager_res, const godot::Ref<godot::Resource>& p_graph_res);

    /**
     * @brief Creates an execution environment that shares memory with another Host.
     */
    void setup_shared(TaskGraphHost* p_target_host, const godot::Ref<TaskGraphResource>& p_graph_res);

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

#endif // IDEAM_GODOT_TASK_GRAPH_HOST_H