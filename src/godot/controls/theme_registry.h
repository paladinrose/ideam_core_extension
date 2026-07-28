#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class ThemeRegistry : public godot::Resource {
    GDCLASS(ThemeRegistry, godot::Resource)

private:
    godot::TypedArray<godot::String> theme_paths;

protected:
    static void _bind_methods();

public:
    ThemeRegistry();
    ~ThemeRegistry();

    // Property getters/setters
    void set_theme_paths(const godot::TypedArray<godot::String>& p_paths);
    godot::TypedArray<godot::String> get_theme_paths() const;

    // Helper for adding a new theme path
    void add_theme_path(const godot::String& p_path);

    // Persistence handling
    static godot::Ref<ThemeRegistry> load_registry(const godot::String& p_registry_path);
    void save_registry(const godot::String& p_registry_path);
};

} // namespace ideam::godot_ext