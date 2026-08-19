#pragma once

#include "../editor/ideam_editor_plugin.h"

#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>

namespace ideam::godot_ext {

class IdeamGraphsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamGraphsPlugin, IdeamEditorPlugin)

private:
    static IdeamGraphsPlugin *singleton;

    // Resource Paths (Deprecated SETTINGS_PATHS in favor of the new ConfigFile ecosystem)
    static constexpr std::string_view GRAPHS_SETTINGS_PATH = "res://addons/ideam_graphs/resources/project_graphs_settings.res";

    // Members
    godot::Ref<godot::EditorInspectorPlugin> graph_editor;
    godot::Window *graph_composer_window = nullptr;

protected:
    static void _bind_methods();

public:
    IdeamGraphsPlugin();
    virtual ~IdeamGraphsPlugin() override;
    
    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // Window Management
    void _on_composer_window_closed();

    // Global Access
#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *undo_redo();
#endif
    // A static bridge to request the shared composer window from the active plugin instance.
    static godot::Window* get_shared_composer_window();
};

} // namespace ideam::godot_ext