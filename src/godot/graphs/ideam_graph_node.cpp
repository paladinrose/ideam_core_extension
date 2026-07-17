#include "ideam_graph_node.h"
#include "ideam_graph_edit.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/button.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamGraphNode::IdeamGraphNode() {
}

IdeamGraphNode::~IdeamGraphNode() {
}

void IdeamGraphNode::_bind_methods() {
    ADD_SIGNAL(MethodInfo("context_clicked", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphNode"), PropertyInfo(Variant::VECTOR2, "at")));
    ADD_SIGNAL(MethodInfo("property_changed", PropertyInfo(Variant::STRING_NAME, "blueprint_id"), PropertyInfo(Variant::STRING_NAME, "property_name"), PropertyInfo(Variant::NIL, "new_value")));
    ADD_SIGNAL(MethodInfo("delete_requested", PropertyInfo(Variant::STRING_NAME, "node_name")));
    ADD_SIGNAL(MethodInfo("duplicate_requested", PropertyInfo(Variant::STRING_NAME, "node_name")));
    ADD_SIGNAL(MethodInfo("connections_requested", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphNode")));

    ClassDB::bind_method(D_METHOD("initialize", "node_res"), &IdeamGraphNode::initialize);
    ClassDB::bind_method(D_METHOD("get_node_resource"), &IdeamGraphNode::get_node_resource);
    ClassDB::bind_method(D_METHOD("get_blueprint_id"), &IdeamGraphNode::get_blueprint_id);
    
    ClassDB::bind_method(D_METHOD("set_locked", "locked"), &IdeamGraphNode::set_locked);
    ClassDB::bind_method(D_METHOD("get_locked"), &IdeamGraphNode::get_locked);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "locked"), "set_locked", "get_locked");
    
    ClassDB::bind_method(D_METHOD("set_error_state", "error"), &IdeamGraphNode::set_error_state);
    ClassDB::bind_method(D_METHOD("get_error_state"), &IdeamGraphNode::get_error_state);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "error_state"), "set_error_state", "get_error_state");
    // ADD_SIGNAL

    ClassDB::bind_method(D_METHOD("set_context_hover", "hovered"), &IdeamGraphNode::set_context_hover);
    ClassDB::bind_method(D_METHOD("get_context_hover"), &IdeamGraphNode::get_context_hover);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_hover"), "set_context_hover", "get_context_hover");
    // ADD_SIGNAL? Leaning against.

    ClassDB::bind_method(D_METHOD("receive_connection_info", "info"), &IdeamGraphNode::receive_connection_info);
    ClassDB::bind_method(D_METHOD("request_connections"), &IdeamGraphNode::request_connections);

    // Register as Godot Properties
    
    
}

void IdeamGraphNode::_ready() {
    // Base GraphNode initialization logic
}

void IdeamGraphNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        if (is_updating_theme) return;

        is_updating_theme = true;
        
        _update_theme_properties(); // Triggers the virtual cascade
        queue_redraw();
        
        is_updating_theme = false;

    }
}

void IdeamGraphNode::_update_theme_properties() {
    // Force refresh all registered left port states
    for (const auto& pair : left_port_states) {
        update_port_state(pair.first, true, pair.second);
    }
    // Force refresh all registered right port states
    for (const auto& pair : right_port_states) {
        update_port_state(pair.first, false, pair.second);
    }
    
    if (is_locked_state) {
        Ref<StyleBox> locked_sb = get_theme_stylebox("panel_locked", "GraphNode");
        add_theme_stylebox_override("panel", locked_sb);
    } else {
        remove_theme_stylebox_override("panel");
    }
    
    lock_btn->set_button_icon(get_theme_icon(is_locked_state ? "node_locked" : "node_unlocked", "GraphNode"));

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
    
    _build_ui();

    update_from_resource(p_node_res);
}

void IdeamGraphNode::update_from_resource(const Ref<IdeamGraphNodeResource>& p_node_res) {
    // If a new resource reference is injected, keep our handle current
    if (node_resource != p_node_res) {
        node_resource = p_node_res;
    }

    if (node_resource.is_null()) return;

    // Direct O(1) state updates from the Authoring Resource
    if (get_name() != node_resource->get_node_name()) {
        set_name(node_resource->get_node_name());
    }
    
    if (get_position_offset() != node_resource->get_position_offset()) {
        set_position_offset(node_resource->get_position_offset());
    }

}

void IdeamGraphNode::_on_lock_toggled() {
    set_locked(!is_locked_state);
}

void IdeamGraphNode::set_locked(bool p_locked) {
    if (is_locked_state == p_locked) return;
    is_locked_state = p_locked;

    if (lock_btn) {
        lock_btn->set_button_icon(get_theme_icon(is_locked_state ? "node_locked" : "node_unlocked", "GraphNode"));
    }
    
    _set_controls_disabled(this, is_locked_state);
    
    // Explicitly notify Godot to trigger our circuit-broken pipeline
    notification(NOTIFICATION_THEME_CHANGED); 
}
bool IdeamGraphNode::get_locked() const { return is_locked_state; }

void IdeamGraphNode::set_error_state(bool p_error) {
    if (is_error_state == p_error) return;
    is_error_state = p_error;
    notification(NOTIFICATION_THEME_CHANGED); 
}
bool IdeamGraphNode::get_error_state() const { return is_error_state; }

void IdeamGraphNode::set_context_hover(bool p_hovered) {
    if (is_context_hovered == p_hovered) return;
    is_context_hovered = p_hovered;
    notification(NOTIFICATION_THEME_CHANGED);
}
bool IdeamGraphNode::get_context_hover() const { return is_context_hovered; }

StringName IdeamGraphNode::get_blueprint_id() const {
    return node_resource.is_valid() ? node_resource->get_node_name() : StringName();
}

void IdeamGraphNode::_build_ui() {
    if (ui_built) return;
    // Floating badge container
    badge_container = memnew(godot::HBoxContainer);
    badge_container->set_name("BadgeContainer");
    
    // Attempt to slot directly into the GraphNode's native titlebar
    godot::HBoxContainer* titlebar = get_titlebar_hbox();
    if (titlebar) {
        titlebar->add_child(badge_container);
    } else {
        add_child(badge_container); // Fallback
    }

    // Lock/Unlock Button
    lock_btn = memnew(godot::Button);
    lock_btn->set_name("LockToggleBtn");
    lock_btn->set_flat(true);
    
    // Initialize icon based on current state
    lock_btn->set_button_icon(get_theme_icon(is_locked_state ? "node_locked" : "node_unlocked", "GraphNode"));
    lock_btn->connect("pressed", godot::Callable(this, "_on_lock_toggled"));
    add_child(lock_btn);
    ui_built = true;
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
        emit_signal("delete_requested", get_node_resource()->get_node_name());
    } else if (p_option_id == 1) {
        emit_signal("duplicate_requested", get_node_resource()->get_node_name());
    }
}


// --- Badge Management ---
void IdeamGraphNode::add_badge(godot::Control* badge) {
    if (badge_container && badge->get_parent() != badge_container) {
        badge_container->add_child(badge);
    }
}

void IdeamGraphNode::remove_badge(godot::Control* badge) {
    if (badge_container && badge->get_parent() == badge_container) {
        badge_container->remove_child(badge);
    }
}

void IdeamGraphNode::clear_badges() {
    if (!badge_container) return;
    for (int i = 0; i < badge_container->get_child_count(); ++i) {
        badge_container->get_child(i)->queue_free();
    }
}

void IdeamGraphNode::request_connections() {
    emit_signal("connections_requested", this);
}

void IdeamGraphNode::receive_connection_info(const godot::Dictionary& p_info) {
    // Base implementation. Virtual, to be overridden by MemoryGraphNode.
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
        case PORT_EMPTY:     return get_theme_color("port_empty_color", "GraphNode");
        case PORT_CONNECTED: return get_theme_color("port_connected_color", "GraphNode");
        case PORT_LOCKED:    return get_theme_color("port_locked_color", "GraphNode");
        case PORT_ERROR:     return get_theme_color("port_error_color", "GraphNode");
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