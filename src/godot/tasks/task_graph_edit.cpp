#include "task_graph_edit.h"
#include <godot_cpp/classes/engine.hpp>

namespace godot {

void TaskGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_execution"), &TaskGraphEdit::start_execution);
    ClassDB::bind_method(D_METHOD("stop_execution"), &TaskGraphEdit::stop_execution);
    
    ADD_SIGNAL(MethodInfo("task_node_selected", PropertyInfo(Variant::INT, "node_id")));
}

TaskGraphEdit::TaskGraphEdit() {
}

TaskGraphEdit::~TaskGraphEdit() {
}

void TaskGraphEdit::_ready() {
    IdeamGraphEdit::_ready();
    
    // Disable editing if we are in a "Running" state logic
    set_show_grid(true);
}

void TaskGraphEdit::set_task_graph(ideam::core::TaskGraph* p_graph) {
    task_graph_ptr = p_graph;
    // Set the base class pointer too so IdeamGraphEdit logic works
    set_core_graph(p_graph); 
    
    if (task_graph_ptr) {
        rebuild_from_core();
    }
}

void TaskGraphEdit::rebuild_from_core() {
    if (!task_graph_ptr) return;

    clear_all_nodes();
    node_map.clear();

    // 1. Create Nodes
    // Note: This assumes IdeamGraph provides a way to iterate current valid nodes
    // For now, we utilize the add_task_node pattern
}

void TaskGraphEdit::_request_connect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port) {
    if (is_running) return;

    TaskGraphNode* v_from = find_visual_node_by_name(p_from_node);
    TaskGraphNode* v_to = find_visual_node_by_name(p_to_node);

    if (v_from && v_to && task_graph_ptr) {
        // Validation Layer: Try to connect in core first
        ideam::core::EdgeID eid = task_graph_ptr->connect_nodes(
            v_from->get_core_node_id(), p_from_port,
            v_to->get_core_node_id(), p_to_port
        );

        // Visual Layer: Only connect if core returned a valid EdgeID (no cycles, etc)
        if (eid != 0xFFFFFFFF) {
            connect_node(p_from_node, p_from_port, p_to_node, p_to_port);
        }
    }
}

void TaskGraphEdit::_request_disconnect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port) {
    if (is_running) return;

    // Note: Disconnection usually needs the EdgeID. 
    // We would look up the edge in the core graph that matches these parameters.
    disconnect_node(p_from_node, p_from_port, p_to_node, p_to_port);
    
    // task_graph_ptr->disconnect_nodes(eid);
}

void TaskGraphEdit::_spawn_node_by_type(int p_type_id) {
    if (!task_graph_ptr) return;

    // 1. Create Core Node
    ideam::core::NodeID cid = task_graph_ptr->add_task_node();
    
    // 2. Create Visual Node
    TaskGraphNode *v_node = memnew(TaskGraphNode);
    v_node->set_graph_context(task_graph_ptr);
    v_node->set_core_node_id(cid);
    
    add_child(v_node);
    v_node->set_position_offset(popup_position + get_scroll_offset());
    
    // 3. Register and Sync
    node_map[cid] = v_node;
    v_node->sync_with_core();
}

TypedArray<String> TaskGraphEdit::_get_new_node_types() const {
    TypedArray<String> arr;
    arr.push_back("Standard Task");
    arr.push_back("Native Optimized Task");
    arr.push_back("Sub-Graph Container");
    return arr;
}

TaskGraphNode* TaskGraphEdit::find_visual_node_by_name(const String& p_name) {
    return Object::cast_to<TaskGraphNode>(get_node_or_null(p_name));
}

void TaskGraphEdit::start_execution() {
    is_running = true;
    // Set GraphEdit to read-only mode visually
    set_modulate(Color(0.7, 0.7, 0.8)); // Slight dimming to indicate "Running"
}

void TaskGraphEdit::stop_execution() {
    is_running = false;
    set_modulate(Color(1, 1, 1));
}

} // namespace godot