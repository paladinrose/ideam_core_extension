#include "game_agent_editor_inspector_plugin.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>

// Assuming GameAgent is defined here, otherwise Godot's Object::cast_to handles the pointer.
#include "../game_entities/game_agent.h" 

namespace ideam::godot_ext {

void GameAgentEditorInspectorPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("_on_new_action_button_pressed"), &GameAgentEditorInspectorPlugin::_on_new_action_button_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("open_new_action_tool_window", "agent", "editor"), &GameAgentEditorInspectorPlugin::open_new_action_tool_window);
    godot::ClassDB::bind_method(godot::D_METHOD("new_action_tool_close"), &GameAgentEditorInspectorPlugin::new_action_tool_close);
    godot::ClassDB::bind_method(godot::D_METHOD("_node_retargeter_change", "new_retargeter"), &GameAgentEditorInspectorPlugin::_node_retargeter_change);
    godot::ClassDB::bind_method(godot::D_METHOD("_signal_connector_change", "new_connector"), &GameAgentEditorInspectorPlugin::_signal_connector_change);
}

GameAgentEditorInspectorPlugin::GameAgentEditorInspectorPlugin() {}

GameAgentEditorInspectorPlugin::~GameAgentEditorInspectorPlugin() {}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *GameAgentEditorInspectorPlugin::get_undo_redo() const {
    // Cleanly fetch the editor's undo/redo manager without storing state
    return godot::EditorInterface::get_singleton()->get_editor_undo_redo();
}
#endif

bool GameAgentEditorInspectorPlugin::_can_handle(godot::Object *p_object) {
    // Safely check if the object is our GameAgent
    return godot::Object::cast_to<GameAgent>(p_object) != nullptr;
}

bool GameAgentEditorInspectorPlugin::_parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) {
    agent = godot::Object::cast_to<GameAgent>(p_object);
    
    if (p_name == "game_pieces") {
        godot::Button* new_action_button = memnew(godot::Button);
        new_action_button->set_text("New Action");
        
        // Connect the button press to our bound class method
        new_action_button->connect("pressed", godot::Callable(this, "_on_new_action_button_pressed"));
        
        add_custom_control(new_action_button);
        
        return false;
    }
    
    return false;
}

void GameAgentEditorInspectorPlugin::_on_new_action_button_pressed() {
    if (agent) {
        open_new_action_tool_window(agent, this);
    }
}

void GameAgentEditorInspectorPlugin::open_new_action_tool_window(GameAgent* p_agent, GameAgentEditorInspectorPlugin* p_editor) {
    // Simulating the GDScript Autoload/Singleton call dynamically
    godot::Object* ideam_games = godot::Engine::get_singleton()->get_singleton("Ideam_Games");
    if (ideam_games) {
        godot::Variant result = ideam_games->call("open_action_tool_window", p_agent, p_editor);
        _tool_window = godot::Object::cast_to<godot::Window>(result);
    }
}

void GameAgentEditorInspectorPlugin::new_action_tool_close() {
    if (_tool_window) {
        _tool_window->emit_signal("close_requested");
    }
}

void GameAgentEditorInspectorPlugin::_node_retargeter_change(godot::Object* new_retargeter) {
    if (!agent) return;

    godot::Variant old_retargeter = agent->get("player_node_assignments");
    
    if (old_retargeter == godot::Variant(new_retargeter)) {
        return;
    }

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager* ur = get_undo_redo();
    if (ur) {
        ur->create_action("Set Player Node Assignments");
        ur->add_do_property(agent, "player_node_assignments", new_retargeter);
        ur->add_undo_property(agent, "player_node_assignments", old_retargeter);
        ur->commit_action();
    }
#endif
}

void GameAgentEditorInspectorPlugin::_signal_connector_change(godot::Object* new_connector) {
    if (!agent) return;

    godot::Variant old_connector = agent->get("player_signal_assignments");
    
    if (old_connector == godot::Variant(new_connector)) {
        return;
    }
    
#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager* ur = get_undo_redo();
    if (ur) {
        ur->create_action("Set Player Signal Assignments");
        ur->add_do_property(agent, "player_signal_assignments", new_connector);
        ur->add_undo_property(agent, "player_signal_assignments", old_connector);
        ur->commit_action();
    }
#endif
}

} // namespace ideam::godot_ext