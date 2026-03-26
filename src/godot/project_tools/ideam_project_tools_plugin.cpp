#include "ideam_project_tools_plugin.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

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
	
	// Ensure the wizard registry exists
	if (!FileAccess::file_exists(String(WIZARD_SETTINGS_PATHS_FILE.data()))) {
		TypedArray<String> initial_paths;
		initial_paths.append(String(DEFAULT_WIZARD_SETTINGS.data()));
		save_settings_paths(initial_paths);
	}

	// Expanded template validation from GDScript logic
	String node_template_dir = String(SCRIPT_TEMPLATE_PATH.data()).path_join("Node");
	Ref<DirAccess> dir = DirAccess::open("res://");
	if (dir.is_valid() && !dir->dir_exists(node_template_dir)) {
		dir->make_dir_recursive(node_template_dir);
		
		String target_file = node_template_dir.path_join("default_template.gd");
		if (!FileAccess::file_exists(target_file)) {
			Ref<FileAccess> src = FileAccess::open(String(DEFAULT_TEMPLATE_SOURCE.data()), FileAccess::READ);
			Ref<FileAccess> dst = FileAccess::open(target_file, FileAccess::WRITE);
			if (src.is_valid() && dst.is_valid()) {
				dst->store_string(src->get_as_text());
			}
		}
	}

	add_tool_menu_item("Ideam/Project Settings Tool", Callable(this, "open_settings_tool"));
	add_tool_menu_item("Ideam/Project Wizard", Callable(this, "open_project_wizard"));
}

void IdeamProjectToolsPlugin::_exit_tree() {
	close_settings_tool();
	close_project_wizard();
	remove_tool_menu_item("Ideam/Project Settings Tool");
	remove_tool_menu_item("Ideam/Project Wizard");
}

Window* IdeamProjectToolsPlugin::create_tool_window(const String &p_title, const String &p_scene_path, Control **out_content) {
	EditorInterface *ei = get_editor_interface();
	Control *main_screen = ei->get_editor_main_screen();
	
	// Find top window
	Window *top = nullptr;
	Node *p = main_screen;
	while (p) {
		top = Object::cast_to<Window>(p);
		if (top && !Object::cast_to<Control>(p)) break; 
		p = p->get_parent();
	}

	Window *win = memnew(Window);
	win->set_title(p_title);
	
	Vector2 min_size = get_viewport()->get_visible_rect().size / 2.0;
	
	ScrollContainer *scroll = memnew(ScrollContainer);
	scroll->set_custom_minimum_size(min_size);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	win->add_child(scroll);

	Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(p_scene_path);
	if (scene.is_valid()) {
		*out_content = Object::cast_to<Control>(scene->instantiate());
		if (*out_content) {
			scroll->add_child(*out_content);
		}
	}

	top->add_child(win);
	win->popup_centered(min_size);
	return win;
}

void IdeamProjectToolsPlugin::open_settings_tool() {
	if (project_settings_window) {
		project_settings_window->grab_focus();
		return;
	}

	Control *content = nullptr;
	project_settings_window = create_tool_window("Ideam Project Settings", String(SETTINGS_TOOL_SCENE.data()), &content);
	project_settings_window->connect("close_requested", Callable(this, "close_settings_tool"));
	
	if (content) {
		// Logic to handle specific tool initialization can go here
		content->connect("close_requested", Callable(this, "close_settings_tool"));
	}
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
		TypedArray<String> paths = get_settings_paths();
		for (int i = 0; i < paths.size(); ++i) {
			String path = paths[i];
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

TypedArray<String> IdeamProjectToolsPlugin::get_settings_paths() {
	TypedArray<String> paths;
	Ref<FileAccess> f = FileAccess::open(String(WIZARD_SETTINGS_PATHS_FILE.data()), FileAccess::READ);
	if (f.is_valid()) {
		while (f->get_position() < f->get_length()) {
			String line = f->get_line().strip_edges();
			if (!line.is_empty()) {
				paths.append(line);
			}
		}
	}
	return paths;
}

void IdeamProjectToolsPlugin::save_settings_paths(const TypedArray<String> &p_paths) {
	Ref<FileAccess> f = FileAccess::open(String(WIZARD_SETTINGS_PATHS_FILE.data()), FileAccess::WRITE);
	if (f.is_valid()) {
		for (int i = 0; i < p_paths.size(); ++i) {
			f->store_line(p_paths[i]);
		}
	}
}

} // namespace godot