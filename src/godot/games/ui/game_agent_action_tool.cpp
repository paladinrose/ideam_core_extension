#include "game_agent_action_tool.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/foldable_container.hpp>

// Project Header Includes
#include "../game_entities/game_agent.h"
#include "../game_entities/actions/game_agent_action.h"
#include "game_agent_action_sequencer.h"
#include "../editor/game_agent_editor_inspector_plugin.h"
#include "../game_entities/game_piece.h"
#include "../game_entities/actions/game_piece_action.h"


namespace ideam::godot_ext {

void GameAgentActionTool::_bind_methods() {
    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("open_tool", "agent", "editor"), &GameAgentActionTool::open_tool, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("new_game_piece"), &GameAgentActionTool::new_game_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("confirm_and_create"), &GameAgentActionTool::confirm_and_create);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_and_close"), &GameAgentActionTool::cancel_and_close);
    godot::ClassDB::bind_method(godot::D_METHOD("close_tool"), &GameAgentActionTool::close_tool);
    godot::ClassDB::bind_method(godot::D_METHOD("_add_piece_action_callback", "piece_actions_list"), &GameAgentActionTool::_add_piece_action_callback);

    // Property Accessors
    godot::ClassDB::bind_method(godot::D_METHOD("set_name_entry", "name_entry"), &GameAgentActionTool::set_name_entry);
    godot::ClassDB::bind_method(godot::D_METHOD("get_name_entry"), &GameAgentActionTool::get_name_entry);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "name_entry", godot::PROPERTY_HINT_NODE_TYPE, "LineEdit"), "set_name_entry", "get_name_entry");

    godot::ClassDB::bind_method(godot::D_METHOD("set_new_piece_button", "new_piece_button"), &GameAgentActionTool::set_new_piece_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_new_piece_button"), &GameAgentActionTool::get_new_piece_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "new_piece_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_new_piece_button", "get_new_piece_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_pieces_list", "game_pieces_list"), &GameAgentActionTool::set_game_pieces_list);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_pieces_list"), &GameAgentActionTool::get_game_pieces_list);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_pieces_list", godot::PROPERTY_HINT_NODE_TYPE, "Container"), "set_game_pieces_list", "get_game_pieces_list");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_pieces_scroll", "game_pieces_scroll"), &GameAgentActionTool::set_game_pieces_scroll);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_pieces_scroll"), &GameAgentActionTool::get_game_pieces_scroll);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_pieces_scroll", godot::PROPERTY_HINT_NODE_TYPE, "ScrollContainer"), "set_game_pieces_scroll", "get_game_pieces_scroll");

    godot::ClassDB::bind_method(godot::D_METHOD("set_confirm_button", "confirm_button"), &GameAgentActionTool::set_confirm_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_confirm_button"), &GameAgentActionTool::get_confirm_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "confirm_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_confirm_button", "get_confirm_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cancel_button", "cancel_button"), &GameAgentActionTool::set_cancel_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cancel_button"), &GameAgentActionTool::get_cancel_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cancel_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_cancel_button", "get_cancel_button");
}

GameAgentActionTool::GameAgentActionTool() {}
GameAgentActionTool::~GameAgentActionTool() {}

// Getters and Setters
void GameAgentActionTool::set_name_entry(godot::LineEdit* p_entry) { name_entry = p_entry; }
godot::LineEdit* GameAgentActionTool::get_name_entry() const { return name_entry; }

void GameAgentActionTool::set_new_piece_button(godot::Button* p_button) { new_piece_button = p_button; }
godot::Button* GameAgentActionTool::get_new_piece_button() const { return new_piece_button; }

void GameAgentActionTool::set_game_pieces_list(godot::Container* p_list) { game_pieces_list = p_list; }
godot::Container* GameAgentActionTool::get_game_pieces_list() const { return game_pieces_list; }

void GameAgentActionTool::set_game_pieces_scroll(godot::ScrollContainer* p_scroll) { game_pieces_scroll = p_scroll; }
godot::ScrollContainer* GameAgentActionTool::get_game_pieces_scroll() const { return game_pieces_scroll; }

void GameAgentActionTool::set_confirm_button(godot::Button* p_button) { confirm_button = p_button; }
godot::Button* GameAgentActionTool::get_confirm_button() const { return confirm_button; }

void GameAgentActionTool::set_cancel_button(godot::Button* p_button) { cancel_button = p_button; }
godot::Button* GameAgentActionTool::get_cancel_button() const { return cancel_button; }

// Class Logic
void GameAgentActionTool::open_tool(godot::Object* agent, godot::Object* editor) {
    _agent = godot::Object::cast_to<GameAgent>(agent);
    _editor = godot::Object::cast_to<GameAgent_EditorInspectorPlugin>(editor);
    
    _new_action = memnew(GameAgentAction);
    
    if (_agent) {
        godot::Array agent_pieces = _agent->call("get_game_pieces");
        for (int i = 0; i < agent_pieces.size(); ++i) {
            _add_piece_entry(agent_pieces[i]);
        }
    }
        
    if (confirm_button && !confirm_button->is_connected("pressed", godot::Callable(this, "confirm_and_create"))) {
        confirm_button->connect("pressed", godot::Callable(this, "confirm_and_create"));
    }
    
    if (cancel_button && !cancel_button->is_connected("pressed", godot::Callable(this, "cancel_and_close"))) {
        cancel_button->connect("pressed", godot::Callable(this, "cancel_and_close"));
    }
}

void GameAgentActionTool::new_game_piece() {
    godot::Array entry_info = _add_piece_entry();
    _new_pieces.append(entry_info);
}

void GameAgentActionTool::confirm_and_create() {
    if (!_check_name()) return;
    if (!_check_new_pieces()) return;
        
    godot::Node* new_owner = _agent->get_tree()->get_edited_scene_root();
    
    _agent->add_child(_new_action);
    _new_action->set_owner(new_owner);
    if (name_entry) _new_action->set_name(name_entry->get_text());
    
    _agent->call("add_action", _new_action);
    
    for (int i = 0; i < _new_pieces.size(); ++i) {
        godot::Array piece_info = _new_pieces[i];
        GamePiece* new_piece = memnew(GamePiece);
        _agent->add_child(new_piece);
        new_piece->set_owner(new_owner);
        
        godot::LineEdit* piece_name_edit = godot::Object::cast_to<godot::LineEdit>(piece_info[4]); // index 4 is name_entry for new pieces
        if (piece_name_edit) new_piece->set_name(piece_name_edit->get_text());
        
        _agent->call("add_game_piece", new_piece);
        
        int action_count = piece_info[2].operator int(); // piece_info[2] is the scroll/count reference
        for (int j = 0; j < action_count; ++j) {
            GamePieceAction* new_piece_action = memnew(GamePieceAction);
            new_piece->add_child(new_piece_action);
            new_piece_action->set_owner(new_owner);
        }
    }
            
    close_tool();
}

void GameAgentActionTool::cancel_and_close() {
    if (_new_action) _new_action->queue_free();
    close_tool();
}

void GameAgentActionTool::close_tool() {
    if (_editor) {
        _editor->call("new_action_tool_close");
    }
}

godot::Array GameAgentActionTool::_add_piece_entry(godot::Object* game_piece) {
    godot::Array entry_info;
    
    godot::Control* new_entry = godot::Object::cast_to<godot::Control>(godot::ClassDB::instantiate("FoldableContainer"));
    entry_info.append(new_entry);
    
    godot::VBoxContainer* entry_container = memnew(godot::VBoxContainer);
    new_entry->add_child(entry_container);
    
    godot::Button* piece_action_button = memnew(godot::Button);
    piece_action_button->set_text("New Piece Action");
    entry_info.append(piece_action_button);
    
    godot::ScrollContainer* piece_actions_scroll = memnew(godot::ScrollContainer);
    entry_container->add_child(piece_actions_scroll);
    entry_info.append(piece_actions_scroll);
    
    godot::VBoxContainer* piece_actions_list = memnew(godot::VBoxContainer);
    piece_actions_scroll->add_child(piece_actions_list);
    entry_info.append(piece_actions_list);
    
    // Connect the callback for adding piece actions
    piece_action_button->connect("pressed", godot::Callable(this, "_add_piece_action_callback").bind(piece_actions_list));
    
    if (game_piece == nullptr) {
        new_entry->set("title", "New Game Piece");
        
        godot::LineEdit* p_name_entry = memnew(godot::LineEdit);
        entry_container->add_child(p_name_entry);
        entry_info.append(p_name_entry);
        
        entry_container->add_child(piece_action_button);
        return entry_info;
    }
    
    GamePiece* gp = godot::Object::cast_to<GamePiece>(game_piece);
    new_entry->set("title", gp->get_name());
    
    godot::CheckBox* include_check = memnew(godot::CheckBox);
    new_entry->call("add_title_bar_control", include_check);
    
    godot::Array actions = gp->call("get_actions");
    for (int i = 0; i < actions.size(); ++i) {
        godot::Object* act = actions[i];
        godot::CheckBox* action_check = memnew(godot::CheckBox);
        action_check->set_text(act->call("get_name"));
        piece_actions_list->add_child(action_check);
    }
    
    entry_container->add_child(piece_action_button);
    if (game_pieces_list) game_pieces_list->add_child(new_entry);
    
    _get_min_y_size();
    
    return entry_info;
}

void GameAgentActionTool::_add_piece_action_callback(godot::VBoxContainer* piece_actions_list) {
    godot::HBoxContainer* new_action_entry = memnew(godot::HBoxContainer);
    piece_actions_list->add_child(new_action_entry);
    
    godot::Label* action_label = memnew(godot::Label);
    action_label->set_text("New Action Name: ");
    new_action_entry->add_child(action_label);
    
    godot::LineEdit* action_name_entry = memnew(godot::LineEdit);
    new_action_entry->add_child(action_name_entry);
}

bool GameAgentActionTool::_check_name() {
    if (!_agent || !name_entry) return false;
    
    godot::Array actions = _agent->call("get_actions");
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* a = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (a && a->get_name() == name_entry->get_text()) {
            godot::UtilityFunctions::printerr("Action Name is already in use.");
            return false;
        }
    }
    return true;
}

bool GameAgentActionTool::_check_new_pieces() {
    for (int i = 0; i < _new_pieces.size(); ++i) {
        continue; // Preserving the explicit 'continue' loop from source
    }
    return true;
}

void GameAgentActionTool::_get_min_y_size() {
    if (!game_pieces_list || !game_pieces_scroll) return;
    
    int c = game_pieces_list->get_child_count() * line_size;
    int v_size = (120 < c) ? 120 : c; // mini(120, c)
    game_pieces_scroll->set_custom_minimum_size(godot::Vector2i(0, v_size));
}

} // namespace ideam::godot_ext