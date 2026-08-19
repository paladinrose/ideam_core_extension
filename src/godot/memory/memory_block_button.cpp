#include "memory_block_button.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryBlockButton::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_buffer_id", "id"), &MemoryBlockButton::set_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_id"), &MemoryBlockButton::get_buffer_id);
    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &MemoryBlockButton::set_selected);
    ClassDB::bind_method(D_METHOD("get_selected"), &MemoryBlockButton::get_selected);
    ClassDB::bind_method(D_METHOD("set_dimmed", "dimmed"), &MemoryBlockButton::set_dimmed);
    ClassDB::bind_method(D_METHOD("get_dimmed"), &MemoryBlockButton::get_dimmed);

    // Signals directed to parent MemoryRibbon
    ADD_SIGNAL(MethodInfo("block_selected", 
        PropertyInfo(Variant::INT, "buffer_id"), 
        PropertyInfo(Variant::BOOL, "shift_pressed"), 
        PropertyInfo(Variant::BOOL, "ctrl_pressed")));

    ADD_SIGNAL(MethodInfo("block_context_menu_requested", 
        PropertyInfo(Variant::VECTOR2, "global_position"), 
        PropertyInfo(Variant::INT, "buffer_id")));

    ADD_SIGNAL(MethodInfo("block_navigated", 
        PropertyInfo(Variant::INT, "buffer_id"), 
        PropertyInfo(Variant::INT, "direction"))); // -1 for Left, +1 for Right
}

MemoryBlockButton::MemoryBlockButton() {
    set_toggle_mode(false);
    set_focus_mode(FOCUS_ALL);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_FILL);
    set_clip_text(true);

    // Internal container for attaching in-block grant icons along the right edge
    grant_icon_container = memnew(HBoxContainer);
    grant_icon_container->set_mouse_filter(MOUSE_FILTER_PASS);
    grant_icon_container->set_alignment(BoxContainer::ALIGNMENT_END);
    grant_icon_container->set_anchors_preset(PRESET_FULL_RECT);
    add_child(grant_icon_container);
}

void MemoryBlockButton::set_buffer_id(int p_id) {
    buffer_id = p_id;
}

void MemoryBlockButton::set_selected(bool p_selected) {
    if (is_selected == p_selected) return;
    is_selected = p_selected;
    _update_visual_state();
}

void MemoryBlockButton::set_dimmed(bool p_dimmed) {
    if (is_dimmed == p_dimmed) return;
    is_dimmed = p_dimmed;
    _update_visual_state();
}

void MemoryBlockButton::_update_visual_state() {
    // Dimming mechanism: reduces opacity without blocking interaction
    if (is_dimmed) {
        set_self_modulate(Color(0.6f, 0.6f, 0.6f, 0.35f));
    } else {
        set_self_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // Visual selection indicator
    if (is_selected) {
        // High-contrast tint / border modulation for active selection
        set_modulate(Color(1.2f, 1.2f, 1.2f, 1.0f));
    } else {
        set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

void MemoryBlockButton::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed()) {
        // Left Click: Selection & Multi-select reporting
        if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
            bool shift = mb->is_shift_pressed();
            bool ctrl = mb->is_command_or_control_pressed();
            emit_signal("block_selected", buffer_id, shift, ctrl);
            accept_event();
            return;
        }

        // Right Click: Forward global position for context menu spawn
        if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT) {
            emit_signal("block_context_menu_requested", mb->get_global_position(), buffer_id);
            accept_event();
            return;
        }
    }

    // Keyboard Arrow Navigation
    Ref<InputEventKey> k = p_event;
    if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
        if (k->get_keycode() == Key::KEY_LEFT) {
            emit_signal("block_navigated", buffer_id, -1);
            accept_event();
            return;
        } else if (k->get_keycode() == Key::KEY_RIGHT) {
            emit_signal("block_navigated", buffer_id, 1);
            accept_event();
            return;
        }
    }

    Button::_gui_input(p_event);
}

Variant MemoryBlockButton::_get_drag_data(const Vector2& p_at_position) {
    Dictionary drag_payload;
    drag_payload["type"] = "memory_buffer_reorder";
    drag_payload["origin_buffer_id"] = buffer_id;

    // Create a lightweight visual preview badge for the cursor
    PanelContainer* preview_panel = memnew(PanelContainer);
    Label* preview_label = memnew(Label);
    preview_label->set_text(get_text());
    preview_panel->add_child(preview_label);

    set_drag_preview(preview_panel);

    return drag_payload;
}

Object* MemoryBlockButton::_make_custom_tooltip(const String& p_for_text) const {
    // Basic buffer tooltip fallback. 
    // This can be expanded or delegated to a specialized BufferTooltipView.
    if (p_for_text.is_empty()) return nullptr;

    PanelContainer* tooltip_panel = memnew(PanelContainer);
    Label* lbl = memnew(Label);
    lbl->set_text(p_for_text);
    tooltip_panel->add_child(lbl);

    return tooltip_panel;
}

} // namespace ideam::godot_ext