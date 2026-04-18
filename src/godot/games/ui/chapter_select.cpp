#include "chapter_select.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

// Assuming these headers exist based on your project structure
#include "../game.h"
#include "../game_entities/game_board.h" 

namespace ideam::godot_ext {

void ChapterSelect::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("chapter_loaded"));
    ADD_SIGNAL(godot::MethodInfo("chapter_selected"));
    ADD_SIGNAL(godot::MethodInfo("canceled"));

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("validate_game"), &ChapterSelect::validate_game);
    godot::ClassDB::bind_method(godot::D_METHOD("build_chapter_selector"), &ChapterSelect::build_chapter_selector);
    godot::ClassDB::bind_method(godot::D_METHOD("select_chapter", "chapterID"), &ChapterSelect::select_chapter);
    godot::ClassDB::bind_method(godot::D_METHOD("load_chapter"), &ChapterSelect::load_chapter);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_chapter_select"), &ChapterSelect::cancel_chapter_select);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_board_label", "game_id"), &ChapterSelect::get_game_board_label);
    
    godot::ClassDB::bind_method(godot::D_METHOD("game_board_already_loaded", "game", "game_board"), &ChapterSelect::game_board_already_loaded);
    godot::ClassDB::bind_method(godot::D_METHOD("game_board_not_found", "game", "game_board_id"), &ChapterSelect::game_board_not_found);
    godot::ClassDB::bind_method(godot::D_METHOD("game_board_load_start", "game", "game_board_id"), &ChapterSelect::game_board_load_start);
    godot::ClassDB::bind_method(godot::D_METHOD("game_board_load_complete", "game", "game_board"), &ChapterSelect::game_board_load_complete);

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_chapters_list", "chapters_list"), &ChapterSelect::set_chapters_list);
    godot::ClassDB::bind_method(godot::D_METHOD("get_chapters_list"), &ChapterSelect::get_chapters_list);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "chapters_list", godot::PROPERTY_HINT_NODE_TYPE, "ItemList"), "set_chapters_list", "get_chapters_list");

    godot::ClassDB::bind_method(godot::D_METHOD("set_load_chapter_button", "load_chapter_button"), &ChapterSelect::set_load_chapter_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_load_chapter_button"), &ChapterSelect::get_load_chapter_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "load_chapter_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_load_chapter_button", "get_load_chapter_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cancel_button", "cancel_button"), &ChapterSelect::set_cancel_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cancel_button"), &ChapterSelect::get_cancel_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cancel_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_cancel_button", "get_cancel_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_message_label", "message_label"), &ChapterSelect::set_message_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_message_label"), &ChapterSelect::get_message_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "message_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_message_label", "get_message_label");
}

ChapterSelect::ChapterSelect() {}

ChapterSelect::~ChapterSelect() {}

void ChapterSelect::_ready() {
    validate_game();
}

// Setters and Getters
void ChapterSelect::set_chapters_list(godot::ItemList* p_list) { chapters_list = p_list; }
godot::ItemList* ChapterSelect::get_chapters_list() const { return chapters_list; }

void ChapterSelect::set_load_chapter_button(godot::Button* p_button) { load_chapter_button = p_button; }
godot::Button* ChapterSelect::get_load_chapter_button() const { return load_chapter_button; }

void ChapterSelect::set_cancel_button(godot::Button* p_button) { cancel_button = p_button; }
godot::Button* ChapterSelect::get_cancel_button() const { return cancel_button; }

void ChapterSelect::set_message_label(godot::Label* p_label) { message_label = p_label; }
godot::Label* ChapterSelect::get_message_label() const { return message_label; }

// Class Functions
void ChapterSelect::validate_game() {
    if (game == nullptr) {
        godot::Node* gameNode = get_parent();
        while (gameNode != nullptr && !gameNode->is_class("Game")) {
            gameNode = gameNode->get_parent();
        }
        
        if (gameNode != nullptr && gameNode->is_class("Game")) {
            Game* newGame = godot::Object::cast_to<Game>(gameNode);
            if (newGame) {
                game = newGame;
                
                game->connect("game_board_already_loaded", godot::Callable(this, "game_board_already_loaded"));
                game->connect("game_board_not_found", godot::Callable(this, "game_board_not_found"));
                game->connect("loading_game_board_started", godot::Callable(this, "game_board_load_start"));
                game->connect("game_board_loaded", godot::Callable(this, "game_board_load_complete"));
                
                build_chapter_selector();
            }
        }
    }
}

void ChapterSelect::build_chapter_selector() {
    if (chapters_list == nullptr) {
        if (has_node("Chapters_List")) {
            chapters_list = get_node<godot::ItemList>("Chapters_List");
        } else {
            chapters_list = memnew(godot::ItemList);
            chapters_list->set_name("Chapters_List");
            add_child(chapters_list);
            chapters_list->set_owner(get_tree()->get_edited_scene_root());
        }
        if (!chapters_list->is_connected("item_selected", godot::Callable(this, "select_chapter"))) {
            chapters_list->connect("item_selected", godot::Callable(this, "select_chapter"));
        }
    }
    
    chapters_list->clear();
    
    if (game == nullptr) {
        return;
    }
        
    int c = 0;
    godot::TypedArray<godot::String> paths = game->get_game_board_paths();
    godot::TypedArray<godot::String> titles = game->get_game_board_titles();
    
    for (int i = 0; i < paths.size(); ++i) {
        godot::String chapter = paths[i];
        if (titles.size() > c) {
            chapters_list->add_item(titles[c]);
        } else {
            chapters_list->add_item(chapter);
        }
        c += 1;
    }
    
    if (load_chapter_button == nullptr) {
        if (has_node("Load_Chapter_Button")) {
            load_chapter_button = get_node<godot::Button>("Load_Chapter_Button");
            if (!load_chapter_button->is_connected("pressed", godot::Callable(this, "load_chapter"))) {
                load_chapter_button->connect("pressed", godot::Callable(this, "load_chapter"));
            }
        } else {
            load_chapter_button = memnew(godot::Button);
            load_chapter_button->set_name("Load_Chapter_Button");
            load_chapter_button->set_text("Load Chapter");
            add_child(load_chapter_button);
            load_chapter_button->set_owner(get_tree()->get_edited_scene_root());
            load_chapter_button->connect("pressed", godot::Callable(this, "load_chapter"));
        }
    }
    
    if (cancel_button == nullptr) {
        if (has_node("Cancel_Button")) {
            cancel_button = get_node<godot::Button>("Cancel_Button");
            if (!cancel_button->is_connected("pressed", godot::Callable(this, "cancel_chapter_select"))) {
                cancel_button->connect("pressed", godot::Callable(this, "cancel_chapter_select"));
            }
        } else {
            cancel_button = memnew(godot::Button);
            cancel_button->set_name("Cancel_Button");
            cancel_button->set_text("Cancel_Button"); // Explicitly matching GDScript typo/intent
            add_child(cancel_button);
            cancel_button->set_owner(get_tree()->get_edited_scene_root());
            cancel_button->connect("pressed", godot::Callable(this, "cancel_chapter_select"));
        }
    }
}

void ChapterSelect::select_chapter(int chapterID) {
    selected_chapter = chapterID;
    
    if (load_chapter_button) {
        if (selected_chapter < 0 || selected_chapter >= game->get_game_board_paths().size()) {
            load_chapter_button->set_disabled(true);
            if (message_label) {
                message_label->set_text("Select a valid chapter to load.");
            }
        } else {
            load_chapter_button->set_disabled(false);
            if (message_label) {
                message_label->set_text("");
            }
        }
    }
        
    emit_signal("chapter_selected");
}

void ChapterSelect::load_chapter() {
    if (selected_chapter < 0) {
        if (load_chapter_button) load_chapter_button->set_disabled(true);
        if (message_label) {
            message_label->set_text("Select a valid chapter to load.");
        }
        return;
    }
        
    emit_signal("chapter_loaded");
    if (game) game->load_game_board(selected_chapter);
}

void ChapterSelect::cancel_chapter_select() {
    emit_signal("canceled");
}

godot::String ChapterSelect::get_game_board_label(int game_id) {
    if (game != nullptr && game_id >= 0 && game_id < game->get_game_board_paths().size()) {
        godot::String game_label = game->get_game_board_paths()[game_id];
        if (game_id < game->get_game_board_titles().size()) {
            game_label = game->get_game_board_titles()[game_id];
        }
        return game_label;
    }
    return "";
}

void ChapterSelect::game_board_already_loaded(godot::Object* p_game, godot::Object* p_game_board) {
    GameBoard* board = godot::Object::cast_to<GameBoard>(p_game_board);
    if (message_label && board) {
        // Assuming GameBoard has a "get_title()" method based on GDScript context
        message_label->set_text(board->call("get_title").operator godot::String() + " is already loaded.");
    }
}

void ChapterSelect::game_board_not_found(godot::Object* p_game, int game_board_id) {
    if (message_label) {
        message_label->set_text("Cannot find: " + get_game_board_label(game_board_id) + ".");
    }
}

void ChapterSelect::game_board_load_start(godot::Object* p_game, int game_board_id) {
    if (message_label) {
        message_label->set_text(get_game_board_label(game_board_id) + " is loading.");
    }
}

void ChapterSelect::game_board_load_complete(godot::Object* p_game, godot::Object* p_game_board) {
    GameBoard* board = godot::Object::cast_to<GameBoard>(p_game_board);
    if (message_label && board) {
        // Assuming GameBoard has a "get_title()" method based on GDScript context
        message_label->set_text(board->call("get_title").operator godot::String() + " is loaded!");
    }
}

} // namespace ideam::godot_ext