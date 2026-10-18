#include "memory_ribbon_button.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryRibbonButton::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_block_type", "type"), &MemoryRibbonButton::set_block_type);
    ClassDB::bind_method(D_METHOD("get_block_type"), &MemoryRibbonButton::get_block_type);
    ClassDB::bind_method(D_METHOD("set_index", "index"), &MemoryRibbonButton::set_index);
    ClassDB::bind_method(D_METHOD("get_index"), &MemoryRibbonButton::get_index);

    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &MemoryRibbonButton::set_selected);
    ClassDB::bind_method(D_METHOD("get_selected"), &MemoryRibbonButton::get_selected);
    ClassDB::bind_method(D_METHOD("set_dimmed", "dimmed"), &MemoryRibbonButton::set_dimmed);
    ClassDB::bind_method(D_METHOD("get_dimmed"), &MemoryRibbonButton::get_dimmed);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "block_type"), "set_block_type", "get_block_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "index"), "set_index", "get_index");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "selected"), "set_selected", "get_selected");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dimmed"), "set_dimmed", "get_dimmed");

    ADD_SIGNAL(MethodInfo("block_selected",
        PropertyInfo(Variant::INT, "block_type"),
        PropertyInfo(Variant::INT, "index"),
        PropertyInfo(Variant::BOOL, "shift_pressed"),
        PropertyInfo(Variant::BOOL, "ctrl_pressed")));

    ADD_SIGNAL(MethodInfo("block_context_menu_requested",
        PropertyInfo(Variant::VECTOR2, "global_position"),
        PropertyInfo(Variant::INT, "block_type"),
        PropertyInfo(Variant::INT, "index")));

    ADD_SIGNAL(MethodInfo("tooltip_requested",
        PropertyInfo(Variant::INT, "block_type"),
        PropertyInfo(Variant::INT, "index"),
        PropertyInfo(Variant::DICTIONARY, "out_data")));
}

MemoryRibbonButton::MemoryRibbonButton() {
    set_toggle_mode(false);
    set_focus_mode(FOCUS_ALL);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_clip_text(true);
}

void MemoryRibbonButton::set_block_type(int p_type) {
    block_type = p_type;
}

void MemoryRibbonButton::set_index(int p_index) {
    index = p_index;
}

void MemoryRibbonButton::set_selected(bool p_selected) {
    if (is_selected == p_selected) return;
    is_selected = p_selected;
    _update_visual_state();
}

void MemoryRibbonButton::set_dimmed(bool p_dimmed) {
    if (is_dimmed == p_dimmed) return;
    is_dimmed = p_dimmed;
    _update_visual_state();
}

void MemoryRibbonButton::_update_visual_state() {
    if (is_dimmed) {
        set_self_modulate(Color(0.6f, 0.6f, 0.6f, 0.35f));
    } else {
        set_self_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    if (is_selected) {
        set_modulate(Color(1.2f, 1.2f, 1.2f, 1.0f));
    } else {
        set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

void MemoryRibbonButton::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed()) {
        if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
            bool shift = mb->is_shift_pressed();
            bool ctrl = mb->is_command_or_control_pressed();
            emit_signal("block_selected", block_type, index, shift, ctrl);
            accept_event();
            return;
        }

        if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT) {
            emit_signal("block_context_menu_requested", mb->get_global_position(), block_type, index);
            accept_event();
            return;
        }
    }

    Button::_gui_input(p_event);
}

Object* MemoryRibbonButton::_make_custom_tooltip(const String& p_for_text) const {
    if (p_for_text.is_empty()) {
        return nullptr;
    }

    Dictionary tooltip_data;
    const_cast<MemoryRibbonButton*>(this)->emit_signal("tooltip_requested", block_type, index, tooltip_data);

    if (tooltip_data.is_empty()) {
        return nullptr;
    }

    if (tooltip_data.has("custom_control")) {
        Object* custom_control = tooltip_data["custom_control"];
        if (custom_control) {
            return custom_control;
        }
    }

    MemoryBufferTooltip* tooltip = memnew(MemoryBufferTooltip);
    tooltip->configure(tooltip_data);
    return tooltip;
}

} // namespace ideam::godot_ext