#include "ideam_graph_node.h"
#include "ideam_graph_edit.h"
#include <godot_cpp/classes/input_event_mouse_button.hpp>

namespace godot {

void IdeamGraphNode::_bind_methods() {
    ADD_SIGNAL(MethodInfo("context_clicked", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphNode"), PropertyInfo(Variant::VECTOR2, "at")));
}

IdeamGraphNode::IdeamGraphNode() {
    // Basic GraphNode setup
    set_resizable(true);
}

IdeamGraphNode::~IdeamGraphNode() {
}

void IdeamGraphNode::_ready() {
    // Standard initialization if needed
}

void IdeamGraphNode::_gui_input(const Ref<InputEvent> &p_event) {
    Ref<InputEventMouseButton> mouse_btn = p_event;

    // Right-click detection
    if (mouse_btn.is_valid() && mouse_btn->get_button_index() == MOUSE_BUTTON_RIGHT && mouse_btn->is_pressed()) {
        _emit_context_request();
        accept_event();
    }
}

void IdeamGraphNode::_emit_context_request() {
    // Instead of just emitting a signal, we find the parent IdeamGraphEdit 
    // to trigger its centralized popup logic.
    IdeamGraphEdit *parent = Object::cast_to<IdeamGraphEdit>(get_parent());
    if (parent) {
        parent->node_context_clicked(this);
    }
    
    // Still emit the signal for any other observers
    emit_signal("context_clicked", this, get_global_position());
}

TypedArray<String> IdeamGraphNode::get_context_menu_options() const {
    // Base implementation provides an empty list
    TypedArray<String> options;
    return options;
}

void IdeamGraphNode::select_context_menu_option(int p_id) {
    // Base implementation - to be overridden by functional nodes
}

} // namespace godot