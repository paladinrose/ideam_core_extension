#include "ideam_editor_plugin.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void IdeamEditorPlugin::_bind_methods() {
	// Base class bindings for the Ideam ecosystem.
}

void IdeamEditorPlugin::validate_script_templates() {
	Ref<DirAccess> dir = DirAccess::open("res://");
	if (dir.is_null()) {
		return;
	}

	String template_path = String(SCRIPT_TEMPLATE_PATH.data());
	if (!dir->dir_exists(template_path)) {
		Error err = dir->make_dir_recursive(template_path);
		if (err == OK) {
			UtilityFunctions::print("Ideam: Created script templates directory at ", template_path);
		}
	}

	String ignore_path = template_path.path_join(String(GDIGNORE_FILENAME.data()));
	if (!FileAccess::file_exists(ignore_path)) {
		Ref<FileAccess> f = FileAccess::open(ignore_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->close();
			UtilityFunctions::print("Ideam: Created .gdignore in templates directory.");
		}
	}
}

void IdeamEditorPlugin::check_for_project_tools(const String &p_settings_path, const String &p_wizard_paths) {
	ResourceLoader *rl = ResourceLoader::get_singleton();
	
	// Ensure the specific plugin settings and the target directory exist
	if (!rl->exists(p_settings_path) || !DirAccess::dir_exists_absolute("res://addons/ideam_project_tools")) {
		return;
	}

	if (!rl->exists(p_wizard_paths)) {
		return;
	}

	Ref<Resource> res = rl->load(p_wizard_paths);
	if (res.is_null()) {
		return;
	}

	// PackedDataContainer stores data in a binary blob. 
	// We use Variant as an intermediary to invoke the internal unpacking to an Array.
	Variant container_as_variant = res;
	Array paths = container_as_variant;
	
	bool already_registered = false;
	for (int i = 0; i < paths.size(); ++i) {
		if (paths[i].operator String() == p_settings_path) {
			already_registered = true;
			break;
		}
	}

	if (!already_registered) {
		paths.append(p_settings_path);
		
		Ref<PackedDataContainer> new_container;
		new_container.instantiate();
		Error pack_err = new_container->pack(paths);
		
		if (pack_err == OK) {
			Error save_err = ResourceSaver::get_singleton()->save(new_container, p_wizard_paths);
			if (save_err == OK) {
				UtilityFunctions::print("Ideam: Registered plugin path: ", p_settings_path);
			}
		}
	}
}

} // namespace godot