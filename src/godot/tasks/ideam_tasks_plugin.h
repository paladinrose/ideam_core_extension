#ifndef IDEAM_TASKS_PLUGIN_H
#define IDEAM_TASKS_PLUGIN_H

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <string_view>

namespace godot {

/**
 * @class IdeamTasksPlugin
 * @brief Ported from Ideam_Tasks.gd. 
 * Manages task-related editor extensions and provides global access to Task undo/redo.
 */
class IdeamTasksPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamTasksPlugin, IdeamEditorPlugin)

private:
    static IdeamTasksPlugin *singleton;

    // Resource Paths
    static constexpr std::string_view TASKS_SETTINGS_PATH = "res://addons/ideam_tasks/resources/project_tasks_settings.res";
    static constexpr std::string_view SETTINGS_PATHS = "res://addons/ideam_project_tools/resources/project_wizard_settings_paths.res";

    // Members
    Ref<EditorInspectorPlugin> plan_editor;

protected:
    static void _bind_methods();

public:
    IdeamTasksPlugin();
    virtual ~IdeamTasksPlugin() override;

    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // Static Accessors
    static Object *undo_redo();
    static void wait_for_editor_frame();

    static IdeamTasksPlugin *get_singleton() { return singleton; }
};

} // namespace godot

#endif // IDEAM_TASKS_PLUGIN_H