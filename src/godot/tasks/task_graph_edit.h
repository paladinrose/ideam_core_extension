#ifndef TASK_GRAPH_EDIT_H
#define TASK_GRAPH_EDIT_H

#include "ideam_graph_edit.h"
#include "task_graph_node.h"
#include "../../core/tasks/task_graph.h"
#include <unordered_map>

namespace godot {

class TaskGraphEdit : public IdeamGraphEdit {
    GDCLASS(TaskGraphEdit, IdeamGraphEdit)

private:
    //Authority reference
    ideam::core::TaskGraph* task_graph_ptr = nullptr;

    // O(1) Lookup: Core NodeID -> Visual Godot Node
    std::unordered_map<ideam::core::NodeID, TaskGraphNode*> node_map;

    bool is_running = false;

protected:
    static void _bind_methods();

    // --- Overrides for Data-Driven Connectivity ---
    void _request_connect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port);
    void _request_disconnect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port);
    
    // --- Node Spawning Overrides ---
    virtual TypedArray<String> _get_new_node_types() const override;
    virtual void _spawn_node_by_type(int p_type_id) override;

public:
    TaskGraphEdit();
    virtual ~TaskGraphEdit() override;

    void _ready() override;

    // --- Core Synchronization ---
    void set_task_graph(ideam::core::TaskGraph* p_graph);
    void rebuild_from_core();
    
    // --- Plan Execution ---
    void start_execution();
    void stop_execution();
    
    // --- Helpers ---
    TaskGraphNode* find_visual_node_by_name(const String& p_name);
};

} // namespace godot

#endif // TASK_GRAPH_EDIT_H