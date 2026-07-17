#include "gameplay_style.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Assuming eventual existence of these headers
#include "gameplay_mode.h"

namespace ideam::godot_ext {

void GameplayStyle::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_modes", "modes"), &GameplayStyle::set_modes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modes"), &GameplayStyle::get_modes);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "modes", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayMode"), "set_modes", "get_modes");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("apply_style", "style"), &GameplayStyle::apply_style);
    godot::ClassDB::bind_method(godot::D_METHOD("save_style"), &GameplayStyle::save_style);
    godot::ClassDB::bind_method(godot::D_METHOD("load_style", "data"), &GameplayStyle::load_style);
}

GameplayStyle::GameplayStyle() {
    // DOD NOTE: Constructors are kept lean to minimize initialization 
    // overhead and support future bulk allocation strategies.
}

GameplayStyle::~GameplayStyle() {
}

void GameplayStyle::set_modes(const godot::TypedArray<GameplayMode> &p_modes) {
    if (p_modes == modes) return;
    modes = p_modes;
    emit_changed();
}

godot::TypedArray<GameplayMode> GameplayStyle::get_modes() const {
    return modes;
}

void GameplayStyle::apply_style(const godot::Ref<GameplayStyle> &p_style) {
    if (!p_style.is_valid()) {
        return;
    }

    godot::TypedArray<GameplayMode> other_modes = p_style->get_modes();
    
    for (int i = 0; i < other_modes.size(); ++i) {
        GameplayMode *style_mode_obj = godot::Object::cast_to<GameplayMode>(other_modes[i]);
        if (!style_mode_obj) continue;

        godot::String other_title = style_mode_obj->get_title();
        bool add_style_mode = true;

        for (int j = 0; j < modes.size(); ++j) {
            GameplayMode *my_mode_obj = godot::Object::cast_to<GameplayMode>(modes[j]);
            if (!my_mode_obj) continue;

            if (my_mode_obj->get_title() == other_title) {
                // DOD NOTE: Calling 'apply_mode' via Object::call is slow due to 
                // method map lookups. Direct virtual or static-typed calls 
                // are preferred for hot-path style merging.
                my_mode_obj->apply_mode(style_mode_obj);
                add_style_mode = false;
                break;
            }
        }

        if (add_style_mode) {
            modes.append(style_mode_obj);
        }
    }
}

godot::Dictionary GameplayStyle::save_style() const {
    godot::Dictionary data;
    // DOD NOTE: JSON/Dictionary serialization creates heavy temporary string 
    // allocations. For large scale data, binary streams are preferred.
    return data;
}

void GameplayStyle::load_style(const godot::Dictionary &p_data) {
    // Placeholder for data restoration logic
}

} // namespace ideam::godot_ext