#include "gameplay_mode.h"
#include <godot_cpp/core/class_db.hpp>

// Assuming eventual existence of these headers in the project
#include "gameplay_perspective.h"
#include "gameplay_difficulty.h"
#include "gameplay_control_map.h"
#include "gameplay_accessibility_option.h"

namespace ideam::godot_ext {

void Gameplay_Mode::_bind_methods() {
    // Perspectives Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_perspectives", "perspectives"), &Gameplay_Mode::set_perspectives);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspectives"), &Gameplay_Mode::get_perspectives);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "perspectives", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayPerspective"), "set_perspectives", "get_perspectives");

    // Difficulties Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulties", "difficulties"), &Gameplay_Mode::set_difficulties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulties"), &Gameplay_Mode::get_difficulties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "difficulties", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayDifficulty"), "set_difficulties", "get_difficulties");

    // Control Maps Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_control_maps", "control_maps"), &Gameplay_Mode::set_control_maps);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_maps"), &Gameplay_Mode::get_control_maps);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "control_maps", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayControlMap"), "set_control_maps", "get_control_maps");

    // Accessibility Options Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_options", "accessibility_options"), &Gameplay_Mode::set_accessibility_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_options"), &Gameplay_Mode::get_accessibility_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "accessibility_options", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayAccessibilityOption"), "set_accessibility_options", "get_accessibility_options");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("apply_mode", "other"), &Gameplay_Mode::apply_mode);
}

Gameplay_Mode::Gameplay_Mode() {
}

Gameplay_Mode::~Gameplay_Mode() {
}

// Setters / Getters
void Gameplay_Mode::set_perspectives(const godot::TypedArray<GameplayPerspective> &p_perspectives) { perspectives = p_perspectives; }
godot::TypedArray<GameplayPerspective> Gameplay_Mode::get_perspectives() const { return perspectives; }

void Gameplay_Mode::set_difficulties(const godot::TypedArray<GameplayDifficulty> &p_difficulties) { difficulties = p_difficulties; }
godot::TypedArray<GameplayDifficulty> Gameplay_Mode::get_difficulties() const { return difficulties; }

void Gameplay_Mode::set_control_maps(const godot::TypedArray<GameplayControlMap> &p_control_maps) { control_maps = p_control_maps; }
godot::TypedArray<GameplayControlMap> Gameplay_Mode::get_control_maps() const { return control_maps; }

void Gameplay_Mode::set_accessibility_options(const godot::TypedArray<GameplayAccessibilityOption> &p_options) { accessibility_options = p_options; }
godot::TypedArray<GameplayAccessibilityOption> Gameplay_Mode::get_accessibility_options() const { return accessibility_options; }

// Class Functions
void Gameplay_Mode::apply_mode(const godot::Ref<Gameplay_Mode> &p_other) {
    if (!p_other.is_valid()) return;

    // Apply Perspectives
    for (int i = 0; i < perspectives.size(); ++i) {
        godot::Object *p_obj = perspectives[i];
        if (!p_obj) continue;
        
        godot::TypedArray<GameplayPerspective> other_perspectives = p_other->get_perspectives();
        for (int j = 0; j < other_perspectives.size(); ++j) {
            godot::Object *op_obj = other_perspectives[j];
            if (op_obj && p_obj->get("title") == op_obj->get("title")) {
                p_obj->call("apply_perspective", op_obj);
            }
        }
    }

    // Apply Difficulties
    for (int i = 0; i < difficulties.size(); ++i) {
        godot::Object *d_obj = difficulties[i];
        if (!d_obj) continue;

        godot::TypedArray<GameplayDifficulty> other_difficulties = p_other->get_difficulties();
        for (int j = 0; j < other_difficulties.size(); ++j) {
            godot::Object *od_obj = other_difficulties[j];
            if (od_obj && d_obj->get("title") == od_obj->get("title")) {
                d_obj->call("apply_difficulty", od_obj);
            }
        }
    }

    // Apply Control Maps
    for (int i = 0; i < control_maps.size(); ++i) {
        godot::Object *cm_obj = control_maps[i];
        if (!cm_obj) continue;

        godot::TypedArray<GameplayControlMap> other_control_maps = p_other->get_control_maps();
        for (int j = 0; j < other_control_maps.size(); ++j) {
            godot::Object *ocm_obj = other_control_maps[j];
            if (ocm_obj && cm_obj->get("title") == ocm_obj->get("title")) {
                cm_obj->call("apply_control_map", ocm_obj);
            }
        }
    }

    // Apply Accessibility Options
    for (int i = 0; i < accessibility_options.size(); ++i) {
        godot::Object *ao_obj = accessibility_options[i];
        if (!ao_obj) continue;

        godot::TypedArray<GameplayAccessibilityOption> other_ao = p_other->get_accessibility_options();
        for (int j = 0; j < other_ao.size(); ++j) {
            godot::Object *oao_obj = other_ao[j];
            if (oao_obj && ao_obj->get("title") == oao_obj->get("title")) {
                ao_obj->call("apply_accessibility_option", oao_obj);
            }
        }
    }
}

} // namespace ideam::godot_ext