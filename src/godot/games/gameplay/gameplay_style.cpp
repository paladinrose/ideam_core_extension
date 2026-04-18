#include "gameplay_style.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Assuming eventual existence of these headers
#include "gameplay_resource.h"
#include "gameplay_mode.h"

namespace ideam::godot_ext {

void GameplayStyle::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_modes", "modes"), &GameplayStyle::set_modes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modes"), &GameplayStyle::get_modes);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "modes", godot::PROPERTY_HINT_TYPE_STRING, "24/34:Gameplay_Mode"), "set_modes", "get_modes");

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

void GameplayStyle::set_modes(const godot::TypedArray<Gameplay_Mode> &p_modes) {
    modes = p_modes;
}

godot::TypedArray<Gameplay_Mode> GameplayStyle::get_modes() const {
    return modes;
}

void GameplayStyle::apply_style(const godot::Ref<GameplayStyle> &p_style) {
    if (!p_style.is_valid()) {
        return;
    }

    godot::TypedArray<Gameplay_Mode> other_modes = p_style->get_modes();
    
    for (int i = 0; i < other_modes.size(); ++i) {
        godot::Object *style_mode_obj = other_modes[i];
        if (!style_mode_obj) continue;

        // Note: Assuming Gameplay_Mode has a 'title' property or 'get_title()' method
        godot::String other_title = style_mode_obj->get("title");
        bool add_style_mode = true;

        for (int j = 0; j < modes.size(); ++j) {
            godot::Object *my_mode_obj = modes[j];
            if (!my_mode_obj) continue;

            if (my_mode_obj->get("title") == other_title) {
                // DOD NOTE: Calling 'apply_mode' via Object::call is slow due to 
                // method map lookups. Direct virtual or static-typed calls 
                // are preferred for hot-path style merging[cite: 108].
                my_mode_obj->call("apply_mode", style_mode_obj);
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
    // allocations. For large scale data, binary streams are preferred[cite: 108].
    return data;
}

void GameplayStyle::load_style(const godot::Dictionary &p_data) {
    // Placeholder for data restoration logic
}

} // namespace ideam::godot_ext