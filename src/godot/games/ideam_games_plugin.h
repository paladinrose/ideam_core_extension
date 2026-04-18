#pragma once

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgentEditorInspectorPlugin;
class GameAgentActionEditorInspectorPlugin;
class GameAgent;

class IdeamGamesPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamGamesPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamGamesPlugin* singleton;

    GameAgentEditorInspectorPlugin* agent_editor = nullptr;
    GameAgentActionEditorInspectorPlugin* agent_action_editor = nullptr;

    godot::Control* agent_action_tool = nullptr;
    godot::Window* agent_action_tool_window = nullptr;

    GameAgent* _current_agent = nullptr;
    godot::Object* _current_editor = nullptr; // Using Object* to avoid circular include issues

    // C++ Constants
    const godot::String GAME_SETTINGS_PATH = "res://addons/ideam_games/resources/agent_action_tool_game_settings.res";
    const godot::String SETTINGS_PATHS = "res://addons/ideam_project_tools/resources/agent_action_tool_settings_paths.txt";
    const godot::String GAME_AGENT_ACTION_TOOL_PATH = "res://addons/ideam_games/scenes/ui/game_agent_action_tool.tscn";

    // Helper functions
    void validate_game_script_templates();
    void _create_template(const godot::String& folder_name, const godot::String& default_file_name);
    void check_for_project_tools();

public:
    IdeamGamesPlugin();
    ~IdeamGamesPlugin();

    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    void open_agent_action_tool();
    void close_agent_action_tool();

    void _scene_change(godot::Node* new_root);

    // Static API exposed to Godot
    static godot::Window* open_action_tool_window(GameAgent* agent, godot::Object* editor);
    static godot::Object* get_static_undo_redo();
};

} // namespace ideam::godot_ext