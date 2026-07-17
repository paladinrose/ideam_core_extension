#include "gameplay_mode.h"
#include <godot_cpp/core/class_db.hpp>

// Assuming eventual existence of these headers in the project
#include "gameplay_perspective.h"
#include "gameplay_difficulty.h"
#include "gameplay_control_map.h"
#include "gameplay_accessibility_option.h"

namespace ideam::godot_ext {

void GameplayMode::_bind_methods() {
    // Perspectives Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_perspectives", "perspectives"), &GameplayMode::set_perspectives);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspectives"), &GameplayMode::get_perspectives);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "perspectives", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayPerspective"), "set_perspectives", "get_perspectives");

    // Difficulties Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulties", "difficulties"), &GameplayMode::set_difficulties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulties"), &GameplayMode::get_difficulties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "difficulties", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayDifficulty"), "set_difficulties", "get_difficulties");

    // Control Maps Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_control_maps", "control_maps"), &GameplayMode::set_control_maps);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_maps"), &GameplayMode::get_control_maps);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "control_maps", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayControlMap"), "set_control_maps", "get_control_maps");

    // Accessibility Options Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_options", "accessibility_options"), &GameplayMode::set_accessibility_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_options"), &GameplayMode::get_accessibility_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "accessibility_options", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayAccessibilityOption"), "set_accessibility_options", "get_accessibility_options");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("apply_mode", "other"), &GameplayMode::apply_mode);
}

GameplayMode::GameplayMode() {
}

GameplayMode::~GameplayMode() {
}

// Setters / Getters
void GameplayMode::set_perspectives(const godot::TypedArray<GameplayPerspective> &p_perspectives) { 
    if (p_perspectives == perspectives) return;
    perspectives = p_perspectives; 
    emit_changed();
}
godot::TypedArray<GameplayPerspective> GameplayMode::get_perspectives() const { return perspectives; }

void GameplayMode::set_difficulties(const godot::TypedArray<GameplayDifficulty> &p_difficulties) { 
    if (p_difficulties == difficulties) return;
    difficulties = p_difficulties; 
    emit_changed();
}
godot::TypedArray<GameplayDifficulty> GameplayMode::get_difficulties() const { return difficulties; }

void GameplayMode::set_control_maps(const godot::TypedArray<GameplayControlMap> &p_control_maps) { 
    if (p_control_maps == control_maps) return;
    control_maps = p_control_maps; 
    emit_changed();
}
godot::TypedArray<GameplayControlMap> GameplayMode::get_control_maps() const { return control_maps; }

void GameplayMode::set_accessibility_options(const godot::TypedArray<GameplayAccessibilityOption> &p_options) { 
    if (p_options == accessibility_options) return;
    accessibility_options = p_options; 
    emit_changed();
}
godot::TypedArray<GameplayAccessibilityOption> GameplayMode::get_accessibility_options() const { return accessibility_options; }

// Class Functions
void GameplayMode::apply_mode(const godot::Ref<GameplayMode> &p_other) {
    if (!p_other.is_valid()) return;

    // Apply Perspectives
    godot::TypedArray<GameplayPerspective> other_perspectives = p_other->get_perspectives();
    for (int i = 0; i < perspectives.size(); ++i) {
        GameplayPerspective *p_obj = godot::Object::cast_to<GameplayPerspective>(perspectives[i]);
        if (!p_obj) continue;
        
        for (int j = 0; j < other_perspectives.size(); ++j) {
            GameplayPerspective *op_obj = godot::Object::cast_to<GameplayPerspective>(other_perspectives[j]);
            if (op_obj && p_obj->get_title() == op_obj->get_title()) {
                p_obj->apply_perspective(op_obj);
            }
        }
    }

    // Apply Difficulties
    godot::TypedArray<GameplayDifficulty> other_difficulties = p_other->get_difficulties();
    for (int i = 0; i < difficulties.size(); ++i) {
        GameplayDifficulty *d_obj = godot::Object::cast_to<GameplayDifficulty>(difficulties[i]);
        if (!d_obj) continue;

        for (int j = 0; j < other_difficulties.size(); ++j) {
            GameplayDifficulty *od_obj = godot::Object::cast_to<GameplayDifficulty>(other_difficulties[j]);
            if (od_obj && d_obj->get_title() == od_obj->get_title()) {
                d_obj->apply_difficulty(od_obj);
            }
        }
    }

    // Apply Control Maps
    godot::TypedArray<GameplayControlMap> other_control_maps = p_other->get_control_maps();
    for (int i = 0; i < control_maps.size(); ++i) {
        GameplayControlMap *cm_obj = godot::Object::cast_to<GameplayControlMap>(control_maps[i]);
        if (!cm_obj) continue;

        for (int j = 0; j < other_control_maps.size(); ++j) {
            GameplayControlMap *ocm_obj = godot::Object::cast_to<GameplayControlMap>(other_control_maps[j]);
            if (ocm_obj && cm_obj->get_title() == ocm_obj->get_title()) {
                cm_obj->apply_control_map(ocm_obj);
            }
        }
    }

    // Apply Accessibility Options
    godot::TypedArray<GameplayAccessibilityOption> other_ao = p_other->get_accessibility_options();
    for (int i = 0; i < accessibility_options.size(); ++i) {
        GameplayAccessibilityOption *ao_obj = godot::Object::cast_to<GameplayAccessibilityOption>(accessibility_options[i]);
        if (!ao_obj) continue;

        for (int j = 0; j < other_ao.size(); ++j) {
            GameplayAccessibilityOption *oao_obj = godot::Object::cast_to<GameplayAccessibilityOption>(other_ao[j]);
            if (oao_obj && ao_obj->get_title() == oao_obj->get_title()) {
                ao_obj->apply_accessibility_option(oao_obj);
            }
        }
    }
}

} // namespace ideam::godot_ext