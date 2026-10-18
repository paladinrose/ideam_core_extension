#include "block_grant_badge.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void BlockGrantBadge::_bind_methods() {
    ClassDB::bind_method(D_METHOD("configure", "grant_index", "grant_name", "access_mode", "has_error", "tooltip_text"), &BlockGrantBadge::configure, DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("get_grant_index"), &BlockGrantBadge::get_grant_index);
    ClassDB::bind_method(D_METHOD("get_access_mode"), &BlockGrantBadge::get_access_mode);
    ClassDB::bind_method(D_METHOD("get_has_error"), &BlockGrantBadge::get_has_error);

    ClassDB::bind_method(D_METHOD("_on_pressed"), &BlockGrantBadge::_on_pressed);
    ClassDB::bind_method(D_METHOD("_on_mouse_entered"), &BlockGrantBadge::_on_mouse_entered);
    ClassDB::bind_method(D_METHOD("_on_mouse_exited"), &BlockGrantBadge::_on_mouse_exited);

    ADD_SIGNAL(MethodInfo("grant_badge_pressed", PropertyInfo(Variant::INT, "grant_index")));
    ADD_SIGNAL(MethodInfo("grant_badge_mouse_entered", PropertyInfo(Variant::INT, "grant_index")));
    ADD_SIGNAL(MethodInfo("grant_badge_mouse_exited", PropertyInfo(Variant::INT, "grant_index")));
}

BlockGrantBadge::BlockGrantBadge() {
    set_focus_mode(FOCUS_NONE);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_custom_minimum_size(Vector2(18, 18));
    set_clip_text(false);

    connect("pressed", Callable(this, "_on_pressed"));
    connect("mouse_entered", Callable(this, "_on_mouse_entered"));
    connect("mouse_exited", Callable(this, "_on_mouse_exited"));
}

void BlockGrantBadge::configure(int p_grant_index, const StringName& p_grant_name, int p_access_mode, bool p_has_error, const String& p_tooltip_text) {
    grant_index = p_grant_index;
    access_mode = p_access_mode;
    has_error = p_has_error;

    set_text(access_mode == 1 ? "W" : "R");

    if (!p_tooltip_text.is_empty()) {
        set_tooltip_text(p_tooltip_text);
    } else {
        String mode_str = (access_mode == 1) ? String("WRITE") : String("READ");
        String name_str = p_grant_name.is_empty() ? (String("Grant ") + String::num_int64(grant_index)) : String(p_grant_name);
        set_tooltip_text(name_str + String(" [") + mode_str + String("]"));
    }

    _update_appearance();
}

void BlockGrantBadge::_update_appearance() {
    Color base_color;
    Color border_color;
    Color text_color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    if (has_error) {
        // Red violation state
        base_color = Color(0.65f, 0.15f, 0.15f, 0.95f);
        border_color = Color(1.0f, 0.35f, 0.35f, 1.0f);
    } else if (access_mode == 1) {
        // Warm Amber / Orange (WRITE claim)
        base_color = Color(0.85f, 0.45f, 0.05f, 0.95f);
        border_color = Color(1.0f, 0.65f, 0.25f, 1.0f);
    } else {
        // Cool Cyan / Sky Blue (READ claim)
        base_color = Color(0.08f, 0.45f, 0.75f, 0.95f);
        border_color = Color(0.35f, 0.75f, 1.0f, 1.0f);
    }

    Ref<StyleBoxFlat> normal_box;
    normal_box.instantiate();
    normal_box->set_bg_color(base_color);
    normal_box->set_border_width_all(1);
    normal_box->set_border_color(border_color);
    normal_box->set_corner_radius_all(3);
    normal_box->set_content_margin_all(0);
    add_theme_stylebox_override("normal", normal_box);

    Ref<StyleBoxFlat> hover_box;
    hover_box.instantiate();
    hover_box->set_bg_color(base_color.lightened(0.2f));
    hover_box->set_border_width_all(1);
    hover_box->set_border_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
    hover_box->set_corner_radius_all(3);
    hover_box->set_content_margin_all(0);
    add_theme_stylebox_override("hover", hover_box);

    Ref<StyleBoxFlat> pressed_box;
    pressed_box.instantiate();
    pressed_box->set_bg_color(base_color.darkened(0.2f));
    pressed_box->set_border_width_all(1);
    pressed_box->set_border_color(border_color);
    pressed_box->set_corner_radius_all(3);
    pressed_box->set_content_margin_all(0);
    add_theme_stylebox_override("pressed", pressed_box);

    add_theme_color_override("font_color", text_color);
    add_theme_color_override("font_hover_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    add_theme_color_override("font_pressed_color", Color(0.85f, 0.85f, 0.85f, 1.0f));
    add_theme_font_size_override("font_size", 10);
}

void BlockGrantBadge::_on_pressed() {
    emit_signal("grant_badge_pressed", grant_index);
}

void BlockGrantBadge::_on_mouse_entered() {
    emit_signal("grant_badge_mouse_entered", grant_index);
}

void BlockGrantBadge::_on_mouse_exited() {
    emit_signal("grant_badge_mouse_exited", grant_index);
}

} // namespace ideam::godot_ext