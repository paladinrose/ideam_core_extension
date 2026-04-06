#ifndef IDEAM_PROJECT_TOOLS_PLUGIN_H
#define IDEAM_PROJECT_TOOLS_PLUGIN_H

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <string_view>

namespace godot {

class IdeamProjectToolsPlugin : public IdeamEditorPlugin {
	GDCLASS(IdeamProjectToolsPlugin, IdeamEditorPlugin)

private:
	// UI State
	Window *project_wizard_window = nullptr;
	Window *project_settings_window = nullptr;

	// Constants for resources
	static constexpr std::string_view SETTINGS_TOOL_SCENE = "res://addons/ideam_project_tools/scenes/project_settings_tool.tscn";
	static constexpr std::string_view WIZARD_SCENE = "res://addons/ideam_project_tools/scenes/project_wizard.tscn";
	static constexpr std::string_view DEFAULT_WIZARD_SETTINGS = "res://addons/ideam_project_tools/resources/default_project_wizard_settings.res";
	static constexpr std::string_view DEFAULT_TEMPLATE_SOURCE = "res://addons/ideam_project_tools/resources/default_template.txt";
    
    // Note: WIZARD_SETTINGS_PATHS_FILE (.txt) has been deprecated in favor of the .ideam_registry.cfg ecosystem.

	// Internal Helpers
	Window* create_tool_window(const String &p_title, const String &p_scene_path, Control **out_content);

protected:
    static void _bind_methods();

public:
    IdeamProjectToolsPlugin() = default;
    ~IdeamProjectToolsPlugin() = default;

	void open_settings_tool();
	void close_settings_tool();
	void open_project_wizard();
	void close_project_wizard();

	virtual void _enter_tree() override;
	virtual void _exit_tree() override;
};

} // namespace godot

#endif // IDEAM_PROJECT_TOOLS_PLUGIN_H