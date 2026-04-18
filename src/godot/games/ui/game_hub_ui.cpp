#include "game_hub_ui.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

// Assuming these project headers exist
#include "../game_hub.h"
#include "../game.h"

namespace ideam::godot_ext {

void GameHubUI::_bind_methods() {
    // Methods for signals/internal calls
    godot::ClassDB::bind_method(godot::D_METHOD("find_game_hub"), &GameHubUI::find_game_hub);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_games_list"), &GameHubUI::validate_games_list);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_selected_game_field"), &GameHubUI::validate_selected_game_field);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_title_label"), &GameHubUI::validate_title_label);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_path_label"), &GameHubUI::validate_path_label);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_game_state_label"), &GameHubUI::validate_game_state_label);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_game_action_button"), &GameHubUI::validate_game_action_button);
    
    godot::ClassDB::bind_method(godot::D_METHOD("build_games_list"), &GameHubUI::build_games_list);
    godot::ClassDB::bind_method(godot::D_METHOD("game_selected", "game_id"), &GameHubUI::game_selected);
    godot::ClassDB::bind_method(godot::D_METHOD("game_action"), &GameHubUI::game_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_state"), &GameHubUI::get_game_state);

    // Property Bindings
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_hub", "game_hub"), &GameHubUI::set_game_hub);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_hub"), &GameHubUI::get_game_hub);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_hub", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameHub"), "set_game_hub", "get_game_hub");

    godot::ClassDB::bind_method(godot::D_METHOD("set_games_list", "games_list"), &GameHubUI::set_games_list);
    godot::ClassDB::bind_method(godot::D_METHOD("get_games_list"), &GameHubUI::get_games_list);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "games_list", godot::PROPERTY_HINT_NODE_TYPE, "ItemList"), "set_games_list", "get_games_list");

    godot::ClassDB::bind_method(godot::D_METHOD("set_selected_game_field", "selected_game_field"), &GameHubUI::set_selected_game_field);
    godot::ClassDB::bind_method(godot::D_METHOD("get_selected_game_field"), &GameHubUI::get_selected_game_field);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "selected_game_field", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_selected_game_field", "get_selected_game_field");

    godot::ClassDB::bind_method(godot::D_METHOD("set_title_label", "title_label"), &GameHubUI::set_title_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title_label"), &GameHubUI::get_title_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "title_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_title_label", "get_title_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_path_label", "path_label"), &GameHubUI::set_path_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_path_label"), &GameHubUI::get_path_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "path_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_path_label", "get_path_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_state_label", "game_state_label"), &GameHubUI::set_game_state_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_state_label"), &GameHubUI::get_game_state_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_state_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_game_state_label", "get_game_state_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_action_button", "game_action_button"), &GameHubUI::set_game_action_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_action_button"), &GameHubUI::get_game_action_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_action_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_game_action_button", "get_game_action_button");
}

GameHubUI::GameHubUI() {}
GameHubUI::~GameHubUI() {}

void GameHubUI::_ready() {
    if (game_hub == nullptr) {
        find_game_hub();
    }
    
    if (game_hub == nullptr) {
        godot::UtilityFunctions::print("No game hub found.");
        return;
    }
        
    validate_games_list();
    validate_selected_game_field();
    validate_title_label();
    validate_path_label();
    validate_game_state_label();
    validate_game_action_button();
    
    build_games_list();
}

// Setters / Getters
void GameHubUI::set_game_hub(GameHub* p_hub) { game_hub = p_hub; }
GameHub* GameHubUI::get_game_hub() const { return game_hub; }

void GameHubUI::set_games_list(godot::ItemList* p_list) { games_list = p_list; }
godot::ItemList* GameHubUI::get_games_list() const { return games_list; }

void GameHubUI::set_selected_game_field(godot::Control* p_field) { selected_game_field = p_field; }
godot::Control* GameHubUI::get_selected_game_field() const { return selected_game_field; }

void GameHubUI::set_title_label(godot::Label* p_label) { title_label = p_label; }
godot::Label* GameHubUI::get_title_label() const { return title_label; }

void GameHubUI::set_path_label(godot::Label* p_label) { path_label = p_label; }
godot::Label* GameHubUI::get_path_label() const { return path_label; }

void GameHubUI::set_game_state_label(godot::Label* p_label) { game_state_label = p_label; }
godot::Label* GameHubUI::get_game_state_label() const { return game_state_label; }

void GameHubUI::set_game_action_button(godot::Button* p_button) { game_action_button = p_button; }
godot::Button* GameHubUI::get_game_action_button() const { return game_action_button; }

// Implementation Functions
void GameHubUI::find_game_hub() {
    godot::Node* gh = get_parent();
    while (gh != nullptr) {
        if (gh->is_class("GameHub")) {
            game_hub = godot::Object::cast_to<GameHub>(gh);
            return;
        }
        gh = gh->get_parent();
    }
}

void GameHubUI::validate_games_list() {
    if (games_list != nullptr) return;
    
    godot::Node* gl = find_child("Games_List", true, false);
    if (gl) {
        games_list = godot::Object::cast_to<godot::ItemList>(gl);
    } else {
        games_list = memnew(godot::ItemList);
        games_list->set_name("Games_List");
        add_child(games_list);
        games_list->set_owner(get_tree()->get_edited_scene_root());
    }
    
    if (!games_list->is_connected("item_selected", godot::Callable(this, "game_selected"))) {
        games_list->connect("item_selected", godot::Callable(this, "game_selected"));
    }
}

void GameHubUI::validate_selected_game_field() {
    if (selected_game_field != nullptr) return;
    
    godot::Node* sgf = find_child("Selected_Game_Field", true, false);
    if (sgf) {
        selected_game_field = godot::Object::cast_to<godot::Control>(sgf);
    } else {
        selected_game_field = memnew(godot::VBoxContainer);
        add_child(selected_game_field);
        selected_game_field->set_owner(get_tree()->get_edited_scene_root());
    }
}

void GameHubUI::validate_title_label() {
    if (title_label != nullptr) return;
    
    godot::Node* tl = find_child("Title_Label", true, false);
    if (tl) {
        title_label = godot::Object::cast_to<godot::Label>(tl);
    } else {
        title_label = memnew(godot::Label);
        title_label->set_name("Title_Label");
        validate_selected_game_field();
        selected_game_field->add_child(title_label);
        title_label->set_owner(get_tree()->get_edited_scene_root());
    }
}

void GameHubUI::validate_path_label() {
    if (path_label != nullptr) return;
    
    godot::Node* tl = find_child("Path_Label", true, false);
    if (tl) {
        path_label = godot::Object::cast_to<godot::Label>(tl);
    } else {
        path_label = memnew(godot::Label);
        path_label->set_name("Path_Label");
        validate_selected_game_field();
        selected_game_field->add_child(path_label);
        path_label->set_owner(get_tree()->get_edited_scene_root());
    }
}

void GameHubUI::validate_game_state_label() {
    if (game_state_label != nullptr) return;
    
    godot::Node* tl = find_child("Game_State_Label", true, false);
    if (tl) {
        game_state_label = godot::Object::cast_to<godot::Label>(tl);
    } else {
        game_state_label = memnew(godot::Label);
        game_state_label->set_name("Game_State_Label");
        validate_selected_game_field();
        selected_game_field->add_child(game_state_label);
        game_state_label->set_owner(get_tree()->get_edited_scene_root());
    }
}

void GameHubUI::validate_game_action_button() {
    if (game_action_button != nullptr) return;
    
    godot::Node* gab = find_child("Game_Action_Button", true, false);
    if (gab) {
        game_action_button = godot::Object::cast_to<godot::Button>(gab);
    } else {
        game_action_button = memnew(godot::Button);
        game_action_button->set_name("Game_Action_Debate"); // Preserve GDScript naming 
        validate_selected_game_field();
        selected_game_field->add_child(game_action_button);
        game_action_button->set_owner(get_tree()->get_edited_scene_root());
    }
    
    if (!game_action_button->is_connected("pressed", godot::Callable(this, "game_action"))) {
        game_action_button->connect("pressed", godot::Callable(this, "game_action"));
    }
}

void GameHubUI::game_selected(int game_id) {
    selected_game_id = game_id;
    if (game_id < 0) {
        if (selected_game_field) selected_game_field->hide();
        return;
    }
    
    if (selected_game_field) selected_game_field->show();
    
    if (game_hub) {
        godot::TypedArray<godot::String> titles = game_hub->call("get_game_titles");
        godot::TypedArray<godot::String> paths = game_hub->call("get_game_paths");
        
        if (title_label && game_id < titles.size()) title_label->set_text(titles[game_id]);
        if (path_label && game_id < paths.size()) path_label->set_text(paths[game_id]);
    }
    get_game_state();
}

void GameHubUI::game_action() {
    if (game_hub == nullptr) return;
    
    switch (action_id) {
        case -1: return;
        case 0:
            game_hub->call("load_game", selected_game_id);
            break;
        case 1:
            game_hub->call("unload_game", selected_game_id);
            break;
    }
}

void GameHubUI::build_games_list() {
    if (games_list == nullptr) return;
    
    games_list->clear();
    if (game_hub == nullptr) return;
        
    godot::TypedArray<godot::String> paths = game_hub->call("get_game_paths");
    float list_height = godot::Math::clamp(35.0f * (float)paths.size(), 30.0f, 150.0f);
    games_list->set_custom_minimum_size(godot::Vector2(0, list_height));
    
    for (int i = 0; i < paths.size(); ++i) {
        games_list->add_item(paths[i]);
    }
}

void GameHubUI::get_game_state() {
    if (game_hub == nullptr) return;
    
    godot::TypedArray<godot::String> paths = game_hub->call("get_game_paths");
    
    if (selected_game_id < 0 || selected_game_id >= paths.size()) {
        if (game_state_label) game_state_label->set_text("Invalid ID");
        action_id = -1;
        if (game_action_button) {
            game_action_button->set_text("Load Game");
            game_action_button->set_disabled(true);
        }
        return;
    }
    
    // Check loaded games dictionary from hub
    godot::Dictionary loaded_games = game_hub->call("get_loaded_games");
    if (loaded_games.has(selected_game_id)) {
        godot::Object* loaded_game_obj = loaded_games[selected_game_id];
        Game* loaded_game = godot::Object::cast_to<Game>(loaded_game_obj);
        
        if (loaded_game != nullptr) {
            // Using enum from Game class port [cite: 2]
            int state = loaded_game->call("get_game_state");
            
            godot::String state_text = "Unknown";
            switch (state) {
                case 0: state_text = "Uninitialized"; break; // GameState::UNINITIALIZED
                case 1: state_text = "Pregame"; break;      // GameState::PREGAME
                case 2: state_text = "Playing"; break;      // GameState::PLAYING
                case 3: state_text = "Paused"; break;       // GameState::PAUSED
                case 4: state_text = "Resolution"; break;   // GameState::RESOLUTION
                case 5: state_text = "Complete"; break;     // GameState::COMPLETE
            }
            
            if (game_state_label) game_state_label->set_text(state_text);
            if (game_action_button) {
                game_action_button->set_text("Unload Game");
                game_action_button->set_disabled(false);
            }
            action_id = 1;
            return;
        }
    }
    
    godot::String game_path = paths[selected_game_id];
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    
    if (!loader->exists(game_path)) {
        if (game_state_label) game_state_label->set_text("Not Found");
        if (game_action_button) {
            game_action_button->set_text("Load Game");
            game_action_button->set_disabled(true);
        }
        action_id = -1;
        return;
    }
    
    int status = loader->load_threaded_get_status(game_path);
    if (status == godot::ResourceLoader::THREAD_LOAD_IN_PROGRESS) {
        if (game_state_label) game_state_label->set_text("Loading");
        if (game_action_button) {
            game_action_button->set_text("Load Game");
            game_action_button->set_disabled(true);
        }
        action_id = -1;
        return;
    }
    
    if (game_state_label) game_state_label->set_text("Not Loaded");
    if (game_action_button) {
        game_action_button->set_text("Load Game");
        game_action_button->set_disabled(false);
    }
    action_id = 0;
}

} // namespace ideam::godot_ext