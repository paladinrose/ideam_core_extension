#include "ideam_editor_plugin.h"

#include <godot_cpp/variant/utility_functions.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void IdeamEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("validate_script_templates"), &IdeamEditorPlugin::validate_script_templates);
	
    // Handshake API
    ClassDB::bind_method(D_METHOD("set_plugin_active", "plugin_name", "active"), &IdeamEditorPlugin::set_plugin_active);
    ClassDB::bind_method(D_METHOD("is_plugin_active", "plugin_name"), &IdeamEditorPlugin::is_plugin_active);

	// Expose the universal registry to GDScript
	ClassDB::bind_method(D_METHOD("register_to_ecosystem", "section", "key", "value"), &IdeamEditorPlugin::register_to_ecosystem);
	ClassDB::bind_method(D_METHOD("get_from_ecosystem", "section", "key", "default_value"), &IdeamEditorPlugin::get_from_ecosystem, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_ecosystem_keys", "section"), &IdeamEditorPlugin::get_ecosystem_keys);
	ClassDB::bind_method(D_METHOD("get_ecosystem_section", "section"), &IdeamEditorPlugin::get_ecosystem_section);
}

void IdeamEditorPlugin::validate_script_templates() {
	Ref<DirAccess> dir = DirAccess::open("res://");
	if (dir.is_null()) return;

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

// --- Plugin Lifecycle & Lateral Handshake ---

void IdeamEditorPlugin::set_plugin_active(const String &p_plugin_name, bool p_active) {
    if (p_active) {
        register_to_ecosystem("ActivePlugins", p_plugin_name, true);
    } else {
        register_to_ecosystem("ActivePlugins", p_plugin_name, false);
    }
}

bool IdeamEditorPlugin::is_plugin_active(const String &p_plugin_name) const {
    return get_from_ecosystem("ActivePlugins", p_plugin_name, false);
}

// --- Universal Ecosystem Registry Implementation ---

void IdeamEditorPlugin::register_to_ecosystem(const String &p_section, const String &p_key, const Variant &p_value) {
	Ref<ConfigFile> config;
	config.instantiate();
	
	String path = String(REGISTRY_PATH.data());
	if (FileAccess::file_exists(path)) {
		config->load(path);
	}

	config->set_value(p_section, p_key, p_value);
	
	Error err = config->save(path);
	if (err != OK) {
		UtilityFunctions::printerr("Ideam: Failed to save to ecosystem registry at ", path);
	}
}

Variant IdeamEditorPlugin::get_from_ecosystem(const String &p_section, const String &p_key, const Variant &p_default) const {
	Ref<ConfigFile> config;
	config.instantiate();
	
	String path = String(REGISTRY_PATH.data());
	if (config->load(path) == OK) {
		return config->get_value(p_section, p_key, p_default);
	}
	
	return p_default;
}

TypedArray<String> IdeamEditorPlugin::get_ecosystem_keys(const String &p_section) const {
	Ref<ConfigFile> config;
	config.instantiate();
	TypedArray<String> keys;

	String path = String(REGISTRY_PATH.data());
	if (config->load(path) == OK && config->has_section(p_section)) {
		PackedStringArray section_keys = config->get_section_keys(p_section);
		for (int i = 0; i < section_keys.size(); ++i) {
			keys.append(section_keys[i]);
		}
	}
	return keys;
}

Dictionary IdeamEditorPlugin::get_ecosystem_section(const String &p_section) const {
	Ref<ConfigFile> config;
	config.instantiate();
	Dictionary section_data;

	String path = String(REGISTRY_PATH.data());
	if (config->load(path) == OK && config->has_section(p_section)) {
		PackedStringArray section_keys = config->get_section_keys(p_section);
		for (int i = 0; i < section_keys.size(); ++i) {
			String key = section_keys[i];
			section_data[key] = config->get_value(p_section, key);
		}
	}
	return section_data;
}

} // namespace ideam::godot_ext