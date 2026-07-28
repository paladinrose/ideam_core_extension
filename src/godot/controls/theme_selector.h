#pragma once

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/theme.hpp>
#include "theme_registry.h"

namespace ideam::godot_ext {

class ThemeSelector : public godot::HBoxContainer {
    GDCLASS(ThemeSelector, godot::HBoxContainer)

private:
    godot::OptionButton* dropdown = nullptr;
    godot::Button* load_btn = nullptr;
    godot::FileDialog* file_dialog = nullptr;

    godot::Ref<ThemeRegistry> registry;
    godot::String registry_path;

    void _refresh_list();

protected:
    static void _bind_methods();

public:
    ThemeSelector();
    ~ThemeSelector();

    // Initialization method to define where this specific tool's registry lives
    void setup(const godot::String& p_registry_path);

    // Signal Handlers extracted from GraphComposer
    void _on_theme_selected(int p_index);
    void _on_load_pressed();
    void _on_file_selected(const godot::String& p_path);
    
    // Helper to programmatically set the theme (e.g., when switching tabs)
    void select_theme(int p_index);
};

} // namespace ideam::godot_ext