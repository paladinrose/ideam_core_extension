#ifndef IDEAM_EDITOR_PLUGIN_H
#define IDEAM_EDITOR_PLUGIN_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/packed_data_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <string_view>

namespace godot {

/**
 * @class IdeamEditorPlugin
 * @brief Abstract base class for the Ideam plugin ecosystem.
 * Handles filesystem standardization and cross-plugin discovery.
 */
class IdeamEditorPlugin : public EditorPlugin {
	GDCLASS(IdeamEditorPlugin, EditorPlugin)

protected:
	static void _bind_methods();

	// C++26 Static Constants
	static constexpr std::string_view SCRIPT_TEMPLATE_PATH = "res://script_templates/";
	static constexpr std::string_view GDIGNORE_FILENAME = ".gdignore";

	/**
	 * @brief Ensures the project has a valid, ignored script_templates directory.
	 */
	void validate_script_templates();

	/**
	 * @brief Registers a plugin's settings path into the global Ideam Project Tools registry.
	 * @param p_settings_path The local plugin settings resource.
	 * @param p_wizard_paths The global registry resource path.
	 */
	void check_for_project_tools(const String &p_settings_path, const String &p_wizard_paths);

public:
	IdeamEditorPlugin() = default;
	virtual ~IdeamEditorPlugin() override = default;
};

} // namespace godot

#endif // IDEAM_EDITOR_PLUGIN_H