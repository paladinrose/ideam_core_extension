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
     * Instantiates its own MemoryManager and compiles the Graph against it.
     */
    void setup_isolated(const godot::Ref<MemoryManagerResource>& p_manager_res, const godot::Ref<TaskGraphResource>& p_graph_res);

    /**
     * @brief Creates an execution environment that shares memory with another Host.
     * Uses the target host's MemoryManager, allowing multiple graphs to process the same contiguous data.
     */
    void setup_shared(TaskGraphHost* p_target_host, const godot::Ref<TaskGraphResource>& p_graph_res);

    /**
     * @brief Ticks the active graph. Derived classes will call this in their preferred process loop.
     */
    void execute_graph(double p_delta);

    /**
     * @brief C++ internal accessor for memory sharing.
     */
    std::shared_ptr<core::MemoryManagerDOD> get_active_manager() const { return active_manager; }
    
    /**
     * @brief Returns true if this host has a valid compiled graph ready to run.
     */
    bool is_ready() const { return active_graph != nullptr && active_manager != nullptr; }
};

} // namespace ideam::godot_ext

#endif // IDEAM_GODOT_TASK_GRAPH_HOST_H