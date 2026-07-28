#include "theme_selector.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void ThemeSelector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_theme_selected", "index"), &ThemeSelector::_on_theme_selected);
    ClassDB::bind_method(D_METHOD("_on_load_pressed"), &ThemeSelector::_on_load_pressed);
    ClassDB::bind_method(D_METHOD("_on_file_selected", "path"), &ThemeSelector::_on_file_selected);

    // This is the new custom signal that the parent tool will listen to
    ADD_SIGNAL(MethodInfo("theme_applied", PropertyInfo(Variant::OBJECT, "theme", PROPERTY_HINT_RESOURCE_TYPE, "Theme")));
}

ThemeSelector::ThemeSelector() {
    set_h_size_flags(SIZE_EXPAND_FILL);

    // UI Construction
    dropdown = memnew(OptionButton);
    dropdown->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(dropdown);

    load_btn = memnew(Button);
    load_btn->set_text("Load Theme");
    add_child(load_btn);

    file_dialog = memnew(FileDialog);
    file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    file_dialog->set_access(FileDialog::ACCESS_RESOURCES);
    
    PackedStringArray filters;
    filters.append("*.tres, *.theme ; Godot Theme Files");
    file_dialog->set_filters(filters);
    add_child(file_dialog);

    // Event Connections
    dropdown->connect("item_selected", Callable(this, "_on_theme_selected"));
    load_btn->connect("pressed", Callable(this, "_on_load_pressed"));
    file_dialog->connect("file_selected", Callable(this, "_on_file_selected"));
}

ThemeSelector::~ThemeSelector() {}

void ThemeSelector::setup(const String& p_registry_path) {
    registry_path = p_registry_path;
    
    // Assuming ThemeRegistry::load_registry has been updated to accept the path
    registry = ThemeRegistry::load_registry(registry_path); 
    _refresh_list();
}

void ThemeSelector::_refresh_list() {
    dropdown->clear();
    if (registry.is_null()) return;

    TypedArray<String> paths = registry->get_theme_paths();
    int valid_theme_count = 0;

    for (int i = 0; i < paths.size(); ++i) {
        String path = paths[i];

        if (path.is_empty() || !ResourceLoader::get_singleton()->exists(path)) {
            UtilityFunctions::printerr("ThemeSelector: Theme path missing or invalid, skipping: ", path);
            continue;
        }

        dropdown->add_item(path.get_file()); 
        int item_idx = dropdown->get_item_count() - 1;
        dropdown->set_item_metadata(item_idx, path);
        valid_theme_count++;
    }

    if (valid_theme_count == 0) {
        dropdown->add_item("No Themes Found");
        dropdown->set_item_metadata(0, ""); 
        dropdown->set_disabled(true);
    } else {
        dropdown->set_disabled(false);
    }
}

void ThemeSelector::_on_theme_selected(int p_index) {
    String theme_path = dropdown->get_item_metadata(p_index);
    Ref<Theme> loaded_theme;

    if (!theme_path.is_empty()) {
        loaded_theme = ResourceLoader::get_singleton()->load(theme_path);
        if (loaded_theme.is_null()) {
            UtilityFunctions::printerr("ThemeSelector: Failed to load theme at ", theme_path);
        }
    }
    
    // Delegate the actual application of the theme to the parent tool
    emit_signal("theme_applied", loaded_theme, p_index);
}

void ThemeSelector::select_theme(int p_index) {
    if (p_index >= 0 && p_index < dropdown->get_item_count()) {
        dropdown->select(p_index);
        _on_theme_selected(p_index);
    }
}

void ThemeSelector::_on_load_pressed() {
    file_dialog->popup_centered_ratio(0.5);
}

void ThemeSelector::_on_file_selected(const String& p_path) {
    if (registry.is_null()) return;
    
    registry->add_theme_path(p_path);
    
    // Assuming ThemeRegistry::save_registry has been updated to accept the path
    registry->save_registry(registry_path); 
    _refresh_list();

    int new_index = dropdown->get_item_count() - 1;
    select_theme(new_index);
}

} // namespace ideam::godot_ext