#pragma once

#include "../editor/ideam_editor_plugin.h"
#include "graph_composer.h"

#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>

namespace ideam::godot_ext {

class IdeamGraphsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamGraphsPlugin, IdeamEditorPlugin)

private:
    static IdeamGraphsPlugin *singleton;

    // Resource Paths (Deprecated SETTINGS_PATHS in favor of the new ConfigFile ecosystem)
    static constexpr std::string_view GRAPHS_SETTINGS_PATH = "res://addons/ideam_graphs/resources/project_graphs_settings.res";
    static constexpr std::string_view GRAPH_COMPOSER_SCENE_PATH = "res://addons/ideam_graphs/scenes/graph_composer_tool.tscn";

    // Members
    godot::Ref<godot::EditorInspectorPlugin> graph_editor;
    godot::Window *graph_composer_window = nullptr;
    GraphComposer *graph_composer = nullptr;

protected:
    static void _bind_methods();

public:
    IdeamGraphsPlugin();
    virtual ~IdeamGraphsPlugin() override;
    
    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // Logic
    void open_graph_composer();
    void close_graph_composer();
    void edit_ideam_graph(godot::Object *p_graph, const godot::Callable &p_graph_close);

    // Global Access
    static godot::Object *undo_redo();
    // A static bridge to request the shared composer window from the active plugin instance.
    static godot::Window* get_shared_composer_window();
};

} // namespace ideam::godot_ext

 // IDEAM_GRAPHS_PLUGIN_H