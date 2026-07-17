#include "theme_registry.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void ThemeRegistry::_bind_methods() {
    // Bind methods for property access
    ClassDB::bind_method(D_METHOD("set_theme_paths", "paths"), &ThemeRegistry::set_theme_paths);
    ClassDB::bind_method(D_METHOD("get_theme_paths"), &ThemeRegistry::get_theme_paths);
    
    // Bind helper method
    ClassDB::bind_method(D_METHOD("add_theme_path", "path"), &ThemeRegistry::add_theme_path);

    // Register the array property so Godot serializes it automatically when save() is called
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "theme_paths", PROPERTY_HINT_ARRAY_TYPE, "String"), "set_theme_paths", "get_theme_paths");
}

ThemeRegistry::ThemeRegistry() {
    // Initialize empty but ready for data
}

ThemeRegistry::~ThemeRegistry() {
}

void ThemeRegistry::set_theme_paths(const TypedArray<String>& p_paths) {
    if (p_paths == theme_paths) return; // Avoid unnecessary updates
    theme_paths = p_paths;
    emit_changed(); // Notify Godot that the resource has changed
}

TypedArray<String> ThemeRegistry::get_theme_paths() const {
    return theme_paths;
}

void ThemeRegistry::add_theme_path(const String& p_path) {
    // Prevent duplicates in our registry
    if (!theme_paths.has(p_path)) {
        theme_paths.append(p_path);
    }
}

Ref<ThemeRegistry> ThemeRegistry::load_registry() {
    String registry_path = "res://addons/ideam_graphs/resources/theme_registry.tres";
    ResourceLoader* loader = ResourceLoader::get_singleton();

    if (loader->exists(registry_path)) {
        return loader->load(registry_path);
    }
    
    // Fallback: If no file exists yet, instantiate a fresh registry
    Ref<ThemeRegistry> new_registry;
    new_registry.instantiate();
    return new_registry;
}

void ThemeRegistry::save_registry() {
    String registry_path = "res://addons/ideam_graphs/resources/theme_registry.tres";
    Error err = ResourceSaver::get_singleton()->save(this, registry_path);
    
    if (err != OK) {
        UtilityFunctions::printerr("ThemeRegistry: Failed to save theme paths to ", registry_path);
    }
}

} // namespace ideam::godot_ext