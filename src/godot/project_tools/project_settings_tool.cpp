#include "project_settings_tool.h"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/packed_data_container.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

ProjectSettingsTool::ProjectSettingsTool() {}
ProjectSettingsTool::~ProjectSettingsTool() {}

void ProjectSettingsTool::_bind_methods() {
	ClassDB::bind_method(D_METHOD("try_save_settings"), &ProjectSettingsTool::try_save_settings);
	ClassDB::bind_method(D_METHOD("load_project_settings"), &ProjectSettingsTool::load_project_settings);
	ClassDB::bind_method(D_METHOD("add_folder_path"), &ProjectSettingsTool::add_folder_path);
	ClassDB::bind_method(D_METHOD("add_script_setting"), &ProjectSettingsTool::add_script_setting);
	ClassDB::bind_method(D_METHOD("add_resource_setting"), &ProjectSettingsTool::add_resource_setting);
	ClassDB::bind_method(D_METHOD("add_scene_setting"), &ProjectSettingsTool::add_scene_setting);

	ADD_SIGNAL(MethodInfo("regained_focus_from_dialog"));
	ADD_SIGNAL(MethodInfo("close_requested"));
}

int ProjectSettingsTool::validate_path(const String &p_path) {
	if (!p_path.is_absolute_path() && !p_path.is_relative_path()) return 1;
	if (!DirAccess::dir_exists_absolute(p_path)) return 2;
	return 0;
}

int ProjectSettingsTool::validate_name(const String &p_name) {
	if (!p_name.is_valid_filename()) return 1;
	return 0;
}

int ProjectSettingsTool::validate_class(const String &p_class) {
	if (ClassDB::class_exists(p_class) && ClassDB::can_instantiate(p_class)) return 0;
	
	// Check global class list (GDScript/GDExtension classes)
	Array global_classes = ProjectSettings::get_singleton()->get_global_class_list();
	for (int i = 0; i < global_classes.size(); ++i) {
		Dictionary element = global_classes[i];
		if (element["class"].operator String() == p_class) return 1;
	}
	return 2;
}

void ProjectSettingsTool::update_full_path_label() {
	if (!full_path_label || !settings_name_field || !settings_path_field) return;

	String path = settings_path_field->get_text();
	String name = settings_name_field->get_text();

	if (path.is_empty()) {
		full_path_label->set_text("res://" + name + ".res");
	} else {
		full_path_label->set_text(path.path_join(name + ".res"));
	}
}

void ProjectSettingsTool::save_project_settings() {
	Dictionary data;
	
	// Folders
	Array folders;
	if (folders_container) {
		for (int i = 0; i < folders_container->get_child_count(); ++i) {
			LineEdit *field = Object::cast_to<LineEdit>(folders_container->get_child(i)->find_child("Path_Field", true, false));
			if (field) folders.append(field->get_text());
		}
	}
	data["folder_paths"] = folders;

	// ... [Other generation logic for scripts, resources, scenes follows the same pattern] ...

	Ref<PackedDataContainer> container;
	container.instantiate();
	container->pack(data);

	String save_path = full_path_label->get_text();
	ResourceSaver::get_singleton()->save(container, save_path);
	
	UtilityFunctions::print("Ideam: Project settings saved to ", save_path);
}

void ProjectSettingsTool::try_save_settings() {
	if (!validate_project_settings()) return;

	update_full_path_label();
	String path = full_path_label->get_text();

	if (FileAccess::file_exists(path)) {
		ConfirmationDialog *confirm = memnew(ConfirmationDialog);
		confirm->set_text("File exists. Overwrite?");
		confirm->connect("confirmed", Callable(this, "save_project_settings"));
		add_child(confirm);
		confirm->popup_centered();
	} else {
		save_project_settings();
	}
}

void ProjectSettingsTool::clear_tool() {
	auto clear_container = [](Control *container) {
		if (!container) return;
		for (int i = 0; i < container->get_child_count(); ++i) {
			container->get_child(i)->queue_free();
		}
	};

	clear_container(folders_container);
	clear_container(scripts_container);
	clear_container(resources_container);
	clear_container(scenes_container);
}

// UI Adder Example
Control* ProjectSettingsTool::add_folder_path() {
	if (folder_path_template.is_null() || !folders_container) return nullptr;

	Control *instance = Object::cast_to<Control>(folder_path_template->instantiate());
	folders_container->add_child(instance);
	
	// Bind UI logic here or via a dedicated C++ wrapper for the sub-item
	return instance;
}

bool ProjectSettingsTool::validate_project_settings() {
	// Reimplementation of the multi-step validation logic
	// In C++, we can use std::ranges for cleaner iteration if using C++20/26 features
	bool is_valid = true;
	
	if (validate_name(settings_name_field->get_text()) > 0) is_valid = false;
	if (validate_path(settings_path_field->get_text()) > 0) is_valid = false;

	return is_valid;
}

} // namespace godot