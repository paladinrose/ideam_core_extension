#include "ideam_project_tools_plugin.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot {

void IdeamProjectToolsPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_settings_tool"), &IdeamProjectToolsPlugin::open_settings_tool);
	ClassDB::bind_method(D_METHOD("close_settings_tool"), &IdeamProjectToolsPlugin::close_settings_tool);
	ClassDB::bind_method(D_METHOD("open_project_wizard"), &IdeamProjectToolsPlugin::open_project_wizard);
	ClassDB::bind_method(D_METHOD("close_project_wizard"), &IdeamProjectToolsPlugin::close_project_wizard);
}

void IdeamProjectToolsPlugin::_enter_tree() {
	// Standard Ideam setup
	validate_script_templates();
	
    // 1. Handshake: Announce this plugin is active
    set_plugin_active("IdeamProjectTools", true);

    // 2. Registry: Register the default wizard settings into the global ecosystem
    // Any other plugin can overwrite or append to "WizardSettings" now.
    register_to_ecosystem("WizardSettings", "Default", String(DEFAULT_WIZARD_SETTINGS.data()));

	// Expanded template validation from GDScript logic
	String base_path = String(SCRIPT_TEMPLATE_PATH.data());
	if (DirAccess::dir_exists_absolute(base_path)) {
		Ref<DirAccess> dir = DirAccess::open(base_path);
		if (dir.is_valid()) {
			dir->list_dir_begin();
			String file_name = dir->get_next();
			while (!file_name.is_empty()) {
				if (dir->current_is_dir() && file_name != "." && file_name != "..") {
					String sub_dir_path = base_path.path_join(file_name);
					Ref<DirAccess> sub_dir = DirAccess::open(sub_dir_path);
					if (sub_dir.is_valid()) {
						bool has_template = false;
						sub_dir->list_dir_begin();
						String sub_file_name = sub_dir->get_next();
						while (!sub_file_name.is_empty()) {
							if (sub_file_name.ends_with(".gd")) {
								has_template = true;
								break;
							}
							sub_file_name = sub_dir->get_next();
						}
						
						if (!has_template) {
							String new_file_path = sub_dir_path.path_join("default_template.gd");
							Ref<FileAccess> source_file = FileAccess::open(String(DEFAULT_TEMPLATE_SOURCE.data()), FileAccess::READ);
							Ref<FileAccess> new_file = FileAccess::open(new_file_path, FileAccess::WRITE);
							if (source_file.is_valid() && new_file.is_valid()) {
								new_file->store_string(source_file->get_as_text());
							}
						}
					}
				}
				file_name = dir->get_next();
			}
		}
	}
}

void IdeamProjectToolsPlugin::_exit_tree() {
	close_project_wizard();
	close_settings_tool();

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamProjectTools", false);
}

Window* IdeamProjectToolsPlugin::create_tool_window(const String &p_title, const String &p_scene_path, Control **out_content) {
	Window *window = memnew(Window);
	window->set_title(p_title);
	
	// Default sizes, matching the GDScript implementation
	window->set_min_size(Vector2i(800, 600));
	window->set_size(Vector2i(800, 600));
	window->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_PRIMARY_SCREEN);

	Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(p_scene_path);
	if (scene.is_valid()) {
		Control *content = Object::cast_to<Control>(scene->instantiate());
		if (content) {
			content->set_anchors_preset(Control::PRESET_FULL_RECT);
			window->add_child(content);
			if (out_content) {
				*out_content = content;
			}
		}
	}

	EditorInterface::get_singleton()->get_editor_main_screen()->add_child(window);
	window->popup();
	
	return window;
}

void IdeamProjectToolsPlugin::open_settings_tool() {
	if (project_settings_window) {
		project_settings_window->grab_focus();
		return;
	}

	Control *content = nullptr;
	project_settings_window = create_tool_window("Ideam Project Settings Builder", String(SETTINGS_TOOL_SCENE.data()), &content);
	project_settings_window->connect("close_requested", Callable(this, "close_settings_tool"));
}

void IdeamProjectToolsPlugin::close_settings_tool() {
	if (project_settings_window) {
		project_settings_window->queue_free();
		project_settings_window = nullptr;
	}
}

void IdeamProjectToolsPlugin::open_project_wizard() {
	if (project_wizard_window) {
		project_wizard_window->grab_focus();
		return;
	}

	Control *content = nullptr;
	project_wizard_window = create_tool_window("Ideam Project Wizard", String(WIZARD_SCENE.data()), &content);
	project_wizard_window->connect("close_requested", Callable(this, "close_project_wizard"));

	if (content) {
        // Query the universal ecosystem for any registered wizard settings
        Dictionary wizard_settings = get_ecosystem_section("WizardSettings");
        Array keys = wizard_settings.keys();
        
        for (int i = 0; i < keys.size(); ++i) {
            String path = wizard_settings[keys[i]];
            if (ResourceLoader::get_singleton()->exists(path)) {
                content->call("unpack_project_settings", path);
            }
        }
        
		content->call("build_project_wizard");
	}
}

void IdeamProjectToolsPlugin::close_project_wizard() {
	if (project_wizard_window) {
		project_wizard_window->queue_free();
		project_wizard_window = nullptr;
	}
}

} // namespace godot