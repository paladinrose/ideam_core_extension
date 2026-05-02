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
    // Fired when the user right-clicks the node
    ADD_SIGNAL(MethodInfo("context_clicked", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphNode"), PropertyInfo(Variant::VECTOR2, "at")));
    
    // Fired when an internal UI element (like a slider or LineEdit) is modified by the user
    ADD_SIGNAL(MethodInfo("property_changed", PropertyInfo(Variant::STRING_NAME, "blueprint_id"), PropertyInfo(Variant::STRING_NAME, "property_name"), PropertyInfo(Variant::NIL, "new_value")));

    ADD_SIGNAL(MethodInfo("delete_request", PropertyInfo(Variant::STRING_NAME, "blueprint_id")));

    ClassDB::bind_method(D_METHOD("initialize", "node_data"), &IdeamGraphNode::initialize);
    ClassDB::bind_method(D_METHOD("get_blueprint_id"), &IdeamGraphNode::get_blueprint_id);
    
}

void IdeamGraphNode::_ready() {
    // Base GraphNode initialization logic
}

void IdeamGraphNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAW) {
        // Evaluate the priority of structural UI states
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
            // Because Godot GraphNode renders its own panel underneath, drawing our custom StyleBox 
            // over the rect acts as the authoritative visual representation of the DOD state.
            draw_style_box(style, Rect2(Point2(0, 0), get_size()));
        }
    }
}

void IdeamGraphNode::_gui_input(const Ref<InputEvent> &p_event) {
    if (is_locked_state) {
        // Reject all interactions if the DOD backbone has locked this node instance
        return;
    }

    Ref<InputEventMouseButton> mouse_btn = p_event;

    // Detect Right-Click for Context Menus
    if (mouse_btn.is_valid() && mouse_btn->get_button_index() == MOUSE_BUTTON_RIGHT && mouse_btn->is_pressed()) {
        _emit_context_request();
        accept_event();
    }
}

void IdeamGraphNode::_emit_context_request() {
    // Route the popup logic centrally through the parent GraphEdit
    IdeamGraphEdit *parent = Object::cast_to<IdeamGraphEdit>(get_parent());
    if (parent) {
        parent->node_context_clicked(this);
    }
    
    // Emit signal for any external listeners
    emit_signal("context_clicked", this, get_global_position());
}

void IdeamGraphNode::initialize(const Dictionary& p_node_data) {
    if (p_node_data.has("name")) {
        blueprint_id = p_node_data["name"];
        set_name(blueprint_id);
    }
    
    if (p_node_data.has("type_id")) {
        type_id = p_node_data["type_id"];
    }

    if (p_node_data.has("properties")) {
        properties = p_node_data["properties"];
    }

    // Trigger child classes to compose their ports and fields
    _build_ui();
}

void IdeamGraphNode::_build_ui() {
    // Base class logic; intended to be overridden by GraphNode specializations 
    // to map memory requirements to visual ports.
}

void IdeamGraphNode::emit_property_changed(const StringName& p_property_name, const Variant& p_new_value) {
    if (is_locked_state) return;
    emit_signal("property_changed", blueprint_id, p_property_name, p_new_value);
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
        emit_signal("delete_request", blueprint_id);
    }
}

// ==========================================
// TIER 1: DOD UI ENFORCEMENT 
// ==========================================

void IdeamGraphNode::set_locked(bool p_locked) {
    if (is_locked_state == p_locked) return;
    
    is_locked_state = p_locked;
    _set_controls_disabled(this, is_locked_state);
    
    queue_redraw(); // Invoke NOTIFICATION_DRAW to update frames
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
    
    // Prevent the root node itself from completely ignoring mouse events (we still need drag capability unless specifically stripped)
    if (control && control != this) {
        control->set_mouse_filter(p_disabled ? Control::MOUSE_FILTER_IGNORE : Control::MOUSE_FILTER_STOP);
        
        // Use generic variant calls to hit common Godot UI interaction states
        if (control->has_method("set_editable")) {
            control->call("set_editable", !p_disabled);
        }
        if (control->has_method("set_disabled")) {
            control->call("set_disabled", p_disabled);
        }
    }

    // Recurse through all child structural containers (e.g., dynamically spawned HBoxContainers for fields)
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
    // Store the logical state for tracking
    if (p_is_left) {
        left_port_states[p_slot_index] = p_state;
    } else {
        right_port_states[p_slot_index] = p_state;
    }

    // Godot GraphNode requires all parameters to be passed to set_slot.
    // We must poll the existing configuration to only modify the target color.
    bool enable_left  = is_slot_enabled_left(p_slot_index);
    int  type_left    = get_slot_type_left(p_slot_index);
    Color color_left  = p_is_left ? _get_color_for_port_state(p_state) : get_slot_color_left(p_slot_index);

    bool enable_right = is_slot_enabled_right(p_slot_index);
    int  type_right   = get_slot_type_right(p_slot_index);
    Color color_right = !p_is_left ? _get_color_for_port_state(p_state) : get_slot_color_right(p_slot_index);

    // Preserve icons if any were assigned natively
    Ref<Texture2D> custom_left = get_slot_custom_icon_left(p_slot_index);
    Ref<Texture2D> custom_right = get_slot_custom_icon_right(p_slot_index);

    set_slot(p_slot_index, 
             enable_left, type_left, color_left, 
             enable_right, type_right, color_right, 
             custom_left, custom_right);
}

} // namespace ideam::godot_ext