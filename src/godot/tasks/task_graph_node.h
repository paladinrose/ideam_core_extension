#ifndef TASK_GRAPH_NODE_H
#define TASK_GRAPH_NODE_H

#include "ideam_graph_node.h"
#include "../../core/tasks/task_graph.h"
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/label.hpp>

namespace godot {

class TaskGraphNode : public IdeamGraphNode {
    GDCLASS(TaskGraphNode, IdeamGraphNode)

private:
    // Core Data References
    ideam::core::TaskGraph* task_graph_ptr = nullptr;
    
    // UI Elements
    VBoxContainer* main_container = nullptr;
    VBoxContainer* input_rows = nullptr;
    VBoxContainer* output_rows = nullptr;

    // Constants for Visual Feedback
    const Color COLOR_PLANNING = Color(0.8, 0.8, 0.8);
    const Color COLOR_IN_PROGRESS = Color(0.2, 0.6, 1.0);
    const Color COLOR_COMPLETE = Color(0.2, 0.9, 0.2);
    const Color COLOR_FAILED = Color(1.0, 0.2, 0.2);

protected:
    static void _bind_methods();

public:
    TaskGraphNode();
    virtual ~TaskGraphNode() override;

    void _ready() override;

    // --- Core Logic ---
    void set_graph_context(ideam::core::TaskGraph* p_graph);
    void sync_with_core();
    void update_status_visuals();

    // --- Port Management ---
    Color get_color_for_type(ideam::core::DataType p_type);

    // --- Overrides ---
    virtual TypedArray<String> get_context_menu_options() const override;
    virtual void select_context_menu_option(int p_id) override;
};

} // namespace godot

#endif // TASK_GRAPH_NODE_H