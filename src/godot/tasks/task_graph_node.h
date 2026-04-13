#ifndef IDEAM_GODOT_TASK_GRAPH_NODE_H
#define IDEAM_GODOT_TASK_GRAPH_NODE_H

#include "../memory/memory_graph_node.h"
#include <godot_cpp/classes/label.hpp>

namespace ideam::godot_ext {

class TaskGraphNode : public MemoryGraphNode {
    GDCLASS(TaskGraphNode, MemoryGraphNode)

private:
    // Core task classification
    uint32_t task_type = 0;

    // Visual indicators
    godot::Label* task_type_label = nullptr;

protected:
    static void _bind_methods();
    virtual void _build_ui() override;

public:
    TaskGraphNode() = default;
    virtual ~TaskGraphNode() override = default;

    uint32_t get_task_type() const { return task_type; }
};

} // namespace ideam::godot_ext

#endif // IDEAM_GODOT_TASK_GRAPH_NODE_H