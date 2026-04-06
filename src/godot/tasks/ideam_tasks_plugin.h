#ifndef IDEAM_TASKS_PLUGIN_H
#define IDEAM_TASKS_PLUGIN_H

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <string_view>

namespace godot {

class IdeamTasksPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamTasksPlugin, IdeamEditorPlugin)

private:
    static IdeamTasksPlugin *singleton;

    // Ecosystem resource paths defining tasks layout defaults
    static constexpr std::string_view TASKS_SETTINGS_PATH = "res://addons/ideam_tasks/resources/project_tasks_settings.res";

    // Inspector for Task resources (TaskGraphResource, etc.)
    Ref<EditorInspectorPlugin> task_inspector;

protected:
    static void _bind_methods();

public:
    IdeamTasksPlugin();
    virtual ~IdeamTasksPlugin() override;

    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // Global Access
    static Object *undo_redo();
};

} // namespace godot

#endif // IDEAM_TASKS_PLUGIN_H