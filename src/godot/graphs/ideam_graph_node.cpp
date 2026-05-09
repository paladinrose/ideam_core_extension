#include "ideam_graph_node.h"
#include "ideam_graph_edit.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamGraphNode::IdeamGraphNode() {
}

IdeamGraphNode::~IdeamGraphNode() {
}

void IdeamGraphNode::_bind_methods() {
    ADD_SIGNAL(MethodInfo("context_clicked", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphNode"), PropertyInfo(Variant::VECTOR2, "at")));
    ADD_SIGNAL(MethodInfo("property_changed", PropertyInfo(Variant::STRING_NAME, "blueprint_id"), PropertyInfo(Variant::STRING_NAME, "property_name"), PropertyInfo(Variant::NIL, "new_value")));
    ADD_SIGNAL(MethodInfo("delete_request", PropertyInfo(Variant::STRING_NAME, "blueprint_id")));

    ClassDB::bind_method(D_METHOD("initialize", "node_res"), &IdeamGraphNode::initialize);
    ClassDB::bind_method(D_METHOD("get_node_resource"), &IdeamGraphNode::get_node_resource);
    ClassDB::bind_method(D_METHOD("get_blueprint_id"), &IdeamGraphNode::get_blueprint_id);
    
    ClassDB::bind_method(D_METHOD("set_locked", "locked"), &IdeamGraphNode::set_locked);
    ClassDB::bind_method(D_METHOD("get_locked"), &IdeamGraphNode::get_locked);
    
    ClassDB::bind_method(D_METHOD("set_error_state", "error"), &IdeamGraphNode::set_error_state);
    ClassDB::bind_method(D_METHOD("get_error_state"), &IdeamGraphNode::get_error_state);
    
    ClassDB::bind_method(D_METHOD("set_context_hover", "hovered"), &IdeamGraphNode::set_context_hover);
    ClassDB::bind_method(D_METHOD("get_context_hover"), &IdeamGraphNode::get_context_hover);

    // Register as Godot Properties
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "locked"), "set_locked", "get_locked");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "error_state"), "set_error_state", "get_error_state");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_hover"), "set_context_hover", "get_context_hover");
}

void IdeamGraphNode::_ready() {
    // Base GraphNode initialization logic
}

void IdeamGraphNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAW) {
        Ref<StyleBox> style;

        if (is_error_state) {
            style = get_theme_stylebox("node_frame_error");
        } else if (is_locked_state) {
            style = get_theme_stylebox("node_frame_locked");
        } else if (is_context_hovered) {
            style = get_theme_stylebox("node_frame_context_hover");
        } else if (is_selected()) {
            style = get_theme_stylebox("node_frame_selected");
        } else {
            style = get_theme_stylebox("node_frame_default");
        }

        if (style.is_valid()) {
            draw_style_box(style, Rect2(Point2(0, 0), get_size()));
        }
    }
}

void IdeamGraphNode::_gui_input(const Ref<InputEvent> &p_event) {
    if (is_locked_state) return;

    Ref<InputEventMouseButton> mouse_btn = p_event;
    if (mouse_btn.is_valid() && mouse_btn->get_button_index() == MOUSE_BUTTON_RIGHT && mouse_btn->is_pressed()) {
        _emit_context_request();
        accept_event();
    }
}

void IdeamGraphNode::_emit_context_request() {
    IdeamGraphEdit *parent = Object::cast_to<IdeamGraphEdit>(get_parent());
    if (parent) {
        parent->node_context_clicked(this);
    }
    emit_signal("context_clicked", this, get_global_position());
}

void IdeamGraphNode::initialize(const Ref<IdeamGraphNodeResource>& p_node_res) {
    node_resource = p_node_res;
    
    if (node_resource.is_valid()) {
        set_name(node_resource->get_node_name());
        set_position_offset(node_resource->get_position_offset());
        
        // Color Application from the visual Editor Metadata
        set_self_modulate(node_resource->get_node_color());
    }

    _build_ui();
}

bool IdeamGraphNode::get_locked() const {
    return is_locked_state;
}

bool IdeamGraphNode::get_error_state() const {
    return is_error_state;
}

bool IdeamGraphNode::get_context_hover() const {
    return is_context_hovered;
}

StringName IdeamGraphNode::get_blueprint_id() const {
    return node_resource.is_valid() ? node_resource->get_node_name() : StringName();
}

void IdeamGraphNode::_build_ui() {
    // Intended to be overridden by polymorphic graph nodes (MemoryGraphNode, TaskGraphNode) 
    // downcasting node_resource to their specific resource types.
}

void IdeamGraphNode::emit_property_changed(const StringName& p_property_name, const Variant& p_new_value) {
    if (is_locked_state) return;
    emit_signal("property_changed", get_blueprint_id(), p_property_name, p_new_value);
}

TypedArray<String> IdeamGraphNode::get_context_menu_options() const {
    TypedArray<String> options;
    if (!is_locked_state) {
        options.push_back("Delete Node");
        options.push_back("Duplicate Node");
    }
    return options;
}

void IdeamGraphNode::select_context_menu_option(int p_option_id) {
    if (p_option_id == 0) {
        emit_signal("delete_request", get_blueprint_id());
    }
}

// DOD Interaction Control
void IdeamGraphNode::set_locked(bool p_locked) {
    if (is_locked_state == p_locked) return;
    is_locked_state = p_locked;
    _set_controls_disabled(this, is_locked_state);
    queue_redraw();
}

void IdeamGraphNode::set_error_state(bool p_error) {
    if (is_error_state == p_error) return;
    is_error_state = p_error;
    queue_redraw();
}

void IdeamGraphNode::set_context_hover(bool p_hovered) {
    if (is_context_hovered == p_hovered) return;
    is_context_hovered = p_hovered;
    queue_redraw();
}

void IdeamGraphNode::_set_controls_disabled(Node* p_node, bool p_disabled) {
    Control* control = Object::cast_to<Control>(p_node);
    if (control && control != this) {
        control->set_mouse_filter(p_disabled ? Control::MOUSE_FILTER_IGNORE : Control::MOUSE_FILTER_STOP);
        if (control->has_method("set_editable")) {
            control->call("set_editable", !p_disabled);
        }
        if (control->has_method("set_disabled")) {
            control->call("set_disabled", p_disabled);
        }
    }
    for (int i = 0; i < p_node->get_child_count(); ++i) {
        _set_controls_disabled(p_node->get_child(i), p_disabled);
    }
}

Color IdeamGraphNode::_get_color_for_port_state(PortState p_state) const {
    switch(p_state) {
        case PORT_EMPTY:     return get_theme_color("port_empty_color");
        case PORT_CONNECTED: return get_theme_color("port_connected_color");
        case PORT_LOCKED:    return get_theme_color("port_locked_color");
        case PORT_ERROR:     return get_theme_color("port_error_color");
        default:             return Color(1, 1, 1, 1);
    }
}

void IdeamGraphNode::update_port_state(int p_slot_index, bool p_is_left, PortState p_state) {
    if (p_is_left) left_port_states[p_slot_index] = p_state;
    else right_port_states[p_slot_index] = p_state;

    bool enable_left  = is_slot_enabled_left(p_slot_index);
    int  type_left    = get_slot_type_left(p_slot_index);
    Color color_left  = p_is_left ? _get_color_for_port_state(p_state) : get_slot_color_left(p_slot_index);

    bool enable_right = is_slot_enabled_right(p_slot_index);
    int  type_right   = get_slot_type_right(p_slot_index);
    Color color_right = !p_is_left ? _get_color_for_port_state(p_state) : get_slot_color_right(p_slot_index);

    Ref<Texture2D> custom_left = get_slot_custom_icon_left(p_slot_index);
    Ref<Texture2D> custom_right = get_slot_custom_icon_right(p_slot_index);

    set_slot(p_slot_index, 
             enable_left, type_left, color_left, 
             enable_right, type_right, color_right, 
             custom_left, custom_right);
}

} // namespace ideam::godot_ext