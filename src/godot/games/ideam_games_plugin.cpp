#include "ideam_games_plugin.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

// Include our previously ported plugins
#include "editor/game_agent_editor_inspector_plugin.h"
#include "editor/game_agent_action_editor_inspector_plugin.h"
#include "game_entities/game_agent.h"

namespace ideam::godot_ext {

IdeamGamesPlugin* IdeamGamesPlugin::singleton = nullptr;

void IdeamGamesPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("open_agent_action_tool"), &IdeamGamesPlugin::open_agent_action_tool);
    godot::ClassDB::bind_method(godot::D_METHOD("close_agent_action_tool"), &IdeamGamesPlugin::close_agent_action_tool);
    godot::ClassDB::bind_method(godot::D_METHOD("_scene_change", "new_root"), &IdeamGamesPlugin::_scene_change);

    // Static bindings so the ecosystem can call these without an instance reference
    godot::ClassDB::bind_static_method("IdeamGamesPlugin", godot::D_METHOD("open_action_tool_window", "agent", "editor"), &IdeamGamesPlugin::open_action_tool_window);
    godot::ClassDB::bind_static_method("IdeamGamesPlugin", godot::D_METHOD("undo_redo"), &IdeamGamesPlugin::get_static_undo_redo);
}

IdeamGamesPlugin::IdeamGamesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamGamesPlugin::~IdeamGamesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamGamesPlugin::_enter_tree() {
    // Call base class template setup
    validate_script_templates(); 
    
    // Call our specialized template setups
    validate_game_script_templates();
    check_for_project_tools();

    // Instantiate native C++ inspector plugins
    agent_editor = memnew(GameAgentEditorInspectorPlugin);
    add_inspector_plugin(agent_editor);

    agent_action_editor = memnew(GameAgentActionEditorInspectorPlugin);
    add_inspector_plugin(agent_action_editor);

    if (!is_connected("scene_changed", godot::Callable(this, "_scene_change"))) {
        connect("scene_changed", godot::Callable(this, "_scene_change"));
    }
}

void IdeamGamesPlugin::_exit_tree() {
    if (is_connected("scene_changed", godot::Callable(this, "_scene_change"))) {
        disconnect("scene_changed", godot::Callable(this, "_scene_change"));
    }

    remove_inspector_plugin(agent_editor);
    remove_inspector_plugin(agent_action_editor);
}

void IdeamGamesPlugin::open_agent_action_tool() {
    godot::Control* top = get_editor_interface()->get_editor_main_screen();
    godot::Control* p = top->get_parent_control();
    while (p != nullptr) {
        top = p;
        p = top->get_parent_control();
    }

    agent_action_tool_window = memnew(godot::Window);
    godot::Vector2 min_size = get_editor_interface()->get_editor_main_screen()->get_viewport()->get_visible_rect().size / 2.0;

    agent_action_tool_window->connect("close_requested", godot::Callable(this, "close_agent_action_tool"));
    agent_action_tool_window->popup_exclusive_centered(top, min_size);

    godot::ScrollContainer* scroll = memnew(godot::ScrollContainer);
    scroll->set_custom_minimum_size(min_size);
    scroll->set_horizontal_scroll_mode(godot::ScrollContainer::SCROLL_MODE_DISABLED);
    agent_action_tool_window->add_child(scroll);

    // Load the tool scene
    godot::Ref<godot::PackedScene> tool_scene = godot::ResourceLoader::get_singleton()->load(GAME_AGENT_ACTION_TOOL_PATH);
    if (tool_scene.is_valid()) {
        agent_action_tool = godot::Object::cast_to<godot::Control>(tool_scene->instantiate());
        if (agent_action_tool) {
            scroll->add_child(agent_action_tool);
            // Dynamic call to handle GDScript integration safely
            agent_action_tool->call("open_tool", _current_agent, _current_editor);
        }
    }
}

void IdeamGamesPlugin::close_agent_action_tool() {
    _current_agent = nullptr;
    _current_editor = nullptr;

    if (agent_action_tool_window) {
        agent_action_tool_window->hide();
        agent_action_tool_window->queue_free();
        agent_action_tool_window = nullptr;
    }
}

void IdeamGamesPlugin::check_for_project_tools() {
    if (godot::ResourceLoader::get_singleton()->exists(GAME_SETTINGS_PATH) && godot::DirAccess::dir_exists_absolute("res://addons/ideam_project_tools")) {
        godot::TypedArray<godot::String> settings_paths;
        godot::Ref<godot::FileAccess> paths_file = godot::FileAccess::open(SETTINGS_PATHS, godot::FileAccess::READ);
        
        bool has_settings = false;

        if (paths_file.is_valid()) {
            while (paths_file->get_position() < paths_file->get_length()) {
                godot::String path = paths_file->get_line();
                if (path == GAME_SETTINGS_PATH) {
                    has_settings = true;
                }
                settings_paths.append(path);
            }
            paths_file->close();
        }

        if (!has_settings) {
            settings_paths.append(GAME_SETTINGS_PATH);
            paths_file = godot::FileAccess::open(SETTINGS_PATHS, godot::FileAccess::WRITE);
            if (paths_file.is_valid()) {
                for (int i = 0; i < settings_paths.size(); ++i) {
                    paths_file->store_line(settings_paths[i]);
                }
                paths_file->close();
            }
        }
    }
}

// DRy (Don't Repeat Yourself) helper to clean up the massive validation block
void IdeamGamesPlugin::_create_template(const godot::String& folder_name, const godot::String& default_file_name) {
    godot::String base_path = "res://script_templates/";
    godot::String folder_path = base_path + folder_name;
    
    godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(base_path);
    if (dir.is_valid() && !dir->dir_exists(folder_name)) {
        dir->make_dir(folder_name);
    }

    godot::Ref<godot::DirAccess> template_dir = godot::DirAccess::open(folder_path);
    if (template_dir.is_valid() && !template_dir->file_exists("default_template.gd")) {
        godot::String source_path = "res://addons/ideam_games/resources/" + default_file_name;
        godot::Ref<godot::FileAccess> default_text = godot::FileAccess::open(source_path, godot::FileAccess::READ);
        
        if (default_text.is_valid()) {
            godot::Ref<godot::FileAccess> new_template = godot::FileAccess::open(folder_path + "/default_template.gd", godot::FileAccess::WRITE);
            if (new_template.is_valid()) {
                new_template->store_string(default_text->get_as_text());
                new_template->close();
            }
            default_text->close();
        }
    }
}

void IdeamGamesPlugin::validate_game_script_templates() {
    _create_template("Game", "default_game_template.txt");
    _create_template("Game_Agent", "default_game_agent_template.txt");
    _create_template("Game_Hub", "default_game_hub_template.txt");
    _create_template("Game_Board", "default_game_board_template.txt");
    _create_template("Game_Entity", "default_game_entity_template.txt");
    _create_template("Game_Piece", "default_game_piece_template.txt");
    _create_template("Game_Player", "default_game_player_template.txt");
}

void IdeamGamesPlugin::_scene_change(godot::Node* new_root) {
    if (agent_action_editor) {
        agent_action_editor->set_editor_root(godot::Object::cast_to<godot::Control>(new_root));
    }
}

// --- Static Godot API ---

godot::Window* IdeamGamesPlugin::open_action_tool_window(GameAgent* agent, godot::Object* editor) {
    if (singleton) {
        singleton->_current_agent = agent;
        singleton->_current_editor = editor;
        singleton->open_agent_action_tool();
        return singleton->agent_action_tool_window;
    }
    return nullptr;
}

godot::Object* IdeamGamesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}

} // namespace ideam::godot_ext