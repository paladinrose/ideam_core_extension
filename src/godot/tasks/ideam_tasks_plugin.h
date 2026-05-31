#pragma once

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamTasksPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamTasksPlugin, IdeamEditorPlugin)

private:
    static IdeamTasksPlugin *singleton;

    // Ecosystem resource paths defining tasks layout defaults
    static constexpr std::string_view TASKS_SETTINGS_PATH = "res://addons/ideam_tasks/resources/project_tasks_settings.res";

    // Inspector for Task resources (TaskGraphResource, etc.)
    godot::Ref<godot::EditorInspectorPlugin> task_inspector;
    
    // Status flag denoting structural safety
    bool manifest_valid = false;

    // Setup helper
    void _check_manifest_and_setup();

protected:
    static void _bind_methods();

public:
    IdeamTasksPlugin();
    virtual ~IdeamTasksPlugin() override;

    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;
    
    // Signal Callbacks
    void _on_manifest_updated();

    // Global Access
    static godot::Object *undo_redo();
};

} // namespace ideam::godot_ext