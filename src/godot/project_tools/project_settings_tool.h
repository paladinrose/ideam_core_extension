#ifndef PROJECT_SETTINGS_TOOL_H
#define PROJECT_SETTINGS_TOOL_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

class ProjectSettingsTool : public Control {
	GDCLASS(ProjectSettingsTool, Control)

private:
	// UI References (Linked via Editor or initialized in C++)
	LineEdit *settings_name_field = nullptr;
	LineEdit *settings_path_field = nullptr;
	Label *full_path_label = nullptr;
	
	Control *folders_container = nullptr;
	Control *scripts_container = nullptr;
	Control *resources_container = nullptr;
	Control *scenes_container = nullptr;

	// Templates
	Ref<PackedScene> folder_path_template;
	Ref<PackedScene> script_setting_template;
	Ref<PackedScene> resource_setting_template;
	Ref<PackedScene> scene_setting_template;
	Ref<PackedScene> scene_sub_node_template;
	Ref<PackedScene> property_template;

	// Internal Logic
	void update_full_path_label();
	int validate_path(const String &p_path);
	int validate_name(const String &p_name);
	int validate_class(const String &p_class);

public:
	// Actions
	void try_save_settings();
	void load_project_settings();
	void save_project_settings();
	void clear_tool();

	// UI Adders
	Control* add_folder_path();
	Control* add_script_setting();
	Control* add_resource_setting();
	Control* add_scene_setting();
	Control* add_sub_node(Control *p_container);

	// Validation
	bool validate_project_settings();

	ProjectSettingsTool();
	~ProjectSettingsTool() override;

protected:
	static void _bind_methods();
};

} // namespace godot

#endif // PROJECT_SETTINGS_TOOL_H