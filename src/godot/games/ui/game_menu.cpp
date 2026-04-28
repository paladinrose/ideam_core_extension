// game_menu.cpp
#include "game_menu.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void GameMenu::_bind_methods() {
    ADD_SIGNAL(godot::MethodInfo("opened"));
    ADD_SIGNAL(godot::MethodInfo("closed"));

    godot::ClassDB::bind_method(godot::D_METHOD("validate_game"), &GameMenu::validate_game);
    godot::ClassDB::bind_method(godot::D_METHOD("build_game_menu"), &GameMenu::build_game_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("close_chapter_select"), &GameMenu::close_chapter_select);
    godot::ClassDB::bind_method(godot::D_METHOD("new_game_pressed"), &GameMenu::new_game_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("continue_game_pressed"), &GameMenu::continue_game_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_pressed"), &GameMenu::load_game_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("save_game_pressed"), &GameMenu::save_game_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("options_pressed"), &GameMenu::options_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("quit_pressed"), &GameMenu::quit_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("open_menu"), &GameMenu::open_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("close_menu"), &GameMenu::close_menu);

    godot::ClassDB::bind_method(godot::D_METHOD("set_game", "game"), &GameMenu::set_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game"), &GameMenu::get_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game"), "set_game", "get_game");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_title", "game_title"), &GameMenu::set_game_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_title"), &GameMenu::get_game_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_title", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_game_title", "get_game_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_new_game_button", "new_game_button"), &GameMenu::set_new_game_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_new_game_button"), &GameMenu::get_new_game_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "new_game_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_new_game_button", "get_new_game_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_continue_game_button", "continue_game_button"), &GameMenu::set_continue_game_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_continue_game_button"), &GameMenu::get_continue_game_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "continue_game_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_continue_game_button", "get_continue_game_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_load_game_button", "load_game_button"), &GameMenu::set_load_game_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_load_game_button"), &GameMenu::get_load_game_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "load_game_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_load_game_button", "get_load_game_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_save_game_button", "save_game_button"), &GameMenu::set_save_game_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_save_game_button"), &GameMenu::get_save_game_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "save_game_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_save_game_button", "get_save_game_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_options_button", "options_button"), &GameMenu::set_options_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_options_button"), &GameMenu::get_options_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "options_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_options_button", "get_options_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_quit_button", "quit_button"), &GameMenu::set_quit_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_quit_button"), &GameMenu::get_quit_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "quit_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_quit_button", "get_quit_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_popup", "popup"), &GameMenu::set_popup);
    godot::ClassDB::bind_method(godot::D_METHOD("get_popup"), &GameMenu::get_popup);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "popup", godot::PROPERTY_HINT_NODE_TYPE, "PopupPanel"), "set_popup", "get_popup");

    godot::ClassDB::bind_method(godot::D_METHOD("set_chapter_select", "chapter_select"), &GameMenu::set_chapter_select);
    godot::ClassDB::bind_method(godot::D_METHOD("get_chapter_select"), &GameMenu::get_chapter_select);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "chapter_select", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_chapter_select", "get_chapter_select");

    godot::ClassDB::bind_method(godot::D_METHOD("set_options_menu", "options_menu"), &GameMenu::set_options_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("get_options_menu"), &GameMenu::get_options_menu);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "options_menu", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_options_menu", "get_options_menu");
}

GameMenu::GameMenu() {}
GameMenu::~GameMenu() {}

void GameMenu::_ready() {
    validate_game();
    build_game_menu();
}

void GameMenu::set_game(Game* p_game) {
    if (p_game != game) {
        game = p_game;
        if (game != nullptr && game_title != nullptr) {
            game_title->set_text(game->get_title());
        }
    }
}
Game* GameMenu::get_game() const { return game; }

void GameMenu::set_game_title(godot::Label* p_label) { game_title = p_label; }
godot::Label* GameMenu::get_game_title() const { return game_title; }

void GameMenu::set_new_game_button(godot::Button* p_button) { new_game_button = p_button; }
godot::Button* GameMenu::get_new_game_button() const { return new_game_button; }

void GameMenu::set_continue_game_button(godot::Button* p_button) { continue_game_button = p_button; }
godot::Button* GameMenu::get_continue_game_button() const { return continue_game_button; }

void GameMenu::set_load_game_button(godot::Button* p_button) { load_game_button = p_button; }
godot::Button* GameMenu::get_load_game_button() const { return load_game_button; }

void GameMenu::set_save_game_button(godot::Button* p_button) { save_game_button = p_button; }
godot::Button* GameMenu::get_save_game_button() const { return save_game_button; }

void GameMenu::set_options_button(godot::Button* p_button) { options_button = p_button; }
godot::Button* GameMenu::get_options_button() const { return options_button; }

void GameMenu::set_quit_button(godot::Button* p_button) { quit_button = p_button; }
godot::Button* GameMenu::get_quit_button() const { return quit_button; }

void GameMenu::set_popup(godot::PopupPanel* p_popup) { popup = p_popup; }
godot::PopupPanel* GameMenu::get_popup() const { return popup; }

void GameMenu::set_chapter_select(ChapterSelect* p_select) { chapter_select = p_select; }
ChapterSelect* GameMenu::get_chapter_select() const { return chapter_select; }

void GameMenu::set_options_menu(GameOptionsMenu* p_menu) { options_menu = p_menu; }
GameOptionsMenu* GameMenu::get_options_menu() const { return options_menu; }

void GameMenu::validate_game() {
    if (game == nullptr) {
        godot::Node* gameNode = get_parent();
        while (gameNode != nullptr && !gameNode->is_class("Game")) {
            gameNode = gameNode->get_parent();
        }
        
        if (gameNode != nullptr && gameNode->is_class("Game")) {
            game = godot::Object::cast_to<Game>(gameNode);
        }
    }
}

void GameMenu::build_game_menu() {
    validate_new_game_button();
    validate_load_game_button();
    validate_continue_game_button();
    validate_save_game_button();
    validate_options_button();
    validate_quit_button();
    validate_popup();
}

void GameMenu::validate_new_game_button() {
    if (new_game_button == nullptr) {
        godot::Node* ngb = get_node_or_null("NewGame");
        if (ngb == nullptr || !ngb->is_class("Button")) {
            new_game_button = memnew(godot::Button);
            new_game_button->set_name("NewGame");
            add_child(new_game_button);
        } else {
            new_game_button = godot::Object::cast_to<godot::Button>(ngb);
        }
    }
    
    if (new_game_button != nullptr && !new_game_button->is_connected("pressed", godot::Callable(this, "new_game_pressed"))) {
        new_game_button->connect("pressed", godot::Callable(this, "new_game_pressed"));
    }
}

void GameMenu::validate_continue_game_button() {
    if (game == nullptr) return;
    
    game->validate_continue_play_state();
    if (game->get_continue_play_state() == "") {
        if (continue_game_button != nullptr) {
            continue_game_button->set_visible(false);
        }
        return;
    }
        
    if (continue_game_button == nullptr) {
        godot::Node* ngb = get_node_or_null("ContinueGame");
        if (ngb == nullptr || !ngb->is_class("Button")) {
            continue_game_button = memnew(godot::Button);
            continue_game_button->set_name("ContinueGame");
            add_child(continue_game_button);
        } else {
            continue_game_button = godot::Object::cast_to<godot::Button>(ngb);
        }
    }
    
    if (continue_game_button != nullptr && !continue_game_button->is_connected("pressed", godot::Callable(this, "continue_game_pressed"))) {
        continue_game_button->connect("pressed", godot::Callable(this, "continue_game_pressed"));
    }
}

void GameMenu::validate_load_game_button() {
    if (game == nullptr) return;
    
    LoadOptions load_opts = game->get_load_options();
    if (load_opts == LoadOptions::NO_LOAD || load_opts == LoadOptions::RESUME_PLAYSTATE) {
        if (load_game_button != nullptr) {
            load_game_button->set_visible(false);
        } else {
            godot::Node* node = get_node_or_null("LoadGame");
            if (node != nullptr) {
                godot::CanvasItem* ci = godot::Object::cast_to<godot::CanvasItem>(node);
                if (ci != nullptr) ci->set_visible(false);
            }
        }
    } else {
        if (load_game_button == nullptr) {
            godot::Node* node = get_node_or_null("LoadGame");
            if (node != nullptr && node->is_class("Button")) {
                load_game_button = godot::Object::cast_to<godot::Button>(node);
            } else {
                load_game_button = memnew(godot::Button);
                load_game_button->set_name("LoadGame");
                add_child(load_game_button);
            }
        }
        if (load_game_button != nullptr && !load_game_button->is_connected("pressed", godot::Callable(this, "load_game_pressed"))) {
            load_game_button->connect("pressed", godot::Callable(this, "load_game_pressed"));
        }
    }
}

void GameMenu::validate_save_game_button() {
    if (game == nullptr) return;
    
    SaveOptions save_opts = game->get_save_options();
    if (save_opts == SaveOptions::NO_SAVE || save_opts == SaveOptions::PLAYSTATE_SAVE) {
        if (save_game_button != nullptr) {
            save_game_button->set_visible(false);
        } else {
            godot::Node* node = get_node_or_null("SaveGame");
            if (node != nullptr) {
                godot::CanvasItem* ci = godot::Object::cast_to<godot::CanvasItem>(node);
                if (ci != nullptr) ci->set_visible(false);
            }
        }
    } else {
        if (save_game_button == nullptr) {
            godot::Node* node = get_node_or_null("SaveGame");
            if (node != nullptr && node->is_class("Button")) {
                save_game_button = godot::Object::cast_to<godot::Button>(node);
            } else {
                save_game_button = memnew(godot::Button);
                save_game_button->set_name("SaveGame");
                add_child(save_game_button);
            }
        }
        if (save_game_button != nullptr && !save_game_button->is_connected("pressed", godot::Callable(this, "save_game_pressed"))) {
            save_game_button->connect("pressed", godot::Callable(this, "save_game_pressed"));
        }
    }
}

void GameMenu::validate_options_button() {
    if (options_menu != nullptr) {
        if (options_button == nullptr) {
            godot::Node* node = get_node_or_null("Options");
            if (node != nullptr && node->is_class("Button")) {
                options_button = godot::Object::cast_to<godot::Button>(node);
            }
        }
    } else {
        if (options_button != nullptr) {
            options_button->hide();
        }
    }
}

void GameMenu::validate_quit_button() {
    if (quit_button == nullptr) {
        godot::Node* node = get_node_or_null("Quit");
        if (node != nullptr && node->is_class("Button")) {
            quit_button = godot::Object::cast_to<godot::Button>(node);
        }
    }
    
    if (quit_button != nullptr && !quit_button->is_connected("pressed", godot::Callable(this, "quit_pressed"))) {
        quit_button->connect("pressed", godot::Callable(this, "quit_pressed"));
    }
}

void GameMenu::validate_popup() {
    if (popup == nullptr) {
        godot::Node* pn = get_node_or_null("PopupPanel");
        if (pn == nullptr || !pn->is_class("PopupPanel")) {
            popup = memnew(godot::PopupPanel);
            popup->set_name("PopupPanel");
            add_child(popup);
        } else {
            popup = godot::Object::cast_to<godot::PopupPanel>(pn);
        }
    }
    
    if (popup != nullptr) {
        popup->set_exclusive(true);
        validate_chapter_select();
    }
}

void GameMenu::validate_chapter_select() {
    if (popup == nullptr) return;

    if (chapter_select == nullptr) {
        godot::Node* chs = popup->get_node_or_null("Chapter_Select");
        if (chs != nullptr) {
            chapter_select = godot::Object::cast_to<ChapterSelect>(chs);
            if (!chapter_select->is_connected("canceled", godot::Callable(this, "close_chapter_select"))) {
                chapter_select->connect("canceled", godot::Callable(this, "close_chapter_select"));
            }
        }
    }
    
    if (chapter_select != nullptr) {
        chapter_select->validate_game();
    }
}

void GameMenu::close_chapter_select() {
    if (popup != nullptr) {
        popup->set_visible(false);
    }
}

void GameMenu::new_game_pressed() {
    if (game == nullptr) return;

    if (chapter_select != nullptr && game->get_new_game_board() < 0) {
        godot::Vector2 viewportSize = get_viewport()->get_visible_rect().size;
        godot::Vector2 sizer = viewportSize * 0.75f;
        if (popup != nullptr) {
            popup->popup_centered(godot::Vector2i(sizer.x, sizer.y));
        }
    } else {
        godot::UtilityFunctions::print("New Game Button Pressed. Starting New Game.");
        game->new_game();
        close_menu();
    }
}

void GameMenu::continue_game_pressed() {
    if (game != nullptr) {
        game->continue_in_progress();
        close_menu();
    }
}

void GameMenu::load_game_pressed() {
    if (game == nullptr) return;
    
    LoadOptions load_opts = game->get_load_options();
    switch (load_opts) {
        case LoadOptions::NO_LOAD:
            return;
            
        case LoadOptions::RESUME_PLAYSTATE:
            game->load_game(0);
            break;
            
        case LoadOptions::LOAD_FILES:
            //Recajigger the chapter_select
            //for this job.  
            //When its Load button is pressed
            //We call
            //game.load_game(chapter_select.selected_chapter)
            return;
            
        case LoadOptions::FULL_LOAD:
            //uhhh, yeah?
            return;
    }
}

void GameMenu::save_game_pressed() {
    if (game == nullptr) return;
        
    SaveOptions save_opts = game->get_save_options();
    switch (save_opts) {
        case SaveOptions::NO_SAVE:
            return;
        case SaveOptions::PLAYSTATE_SAVE:
            game->save_game();
            break;
    }
}

void GameMenu::options_pressed() {
    if (game == nullptr || options_menu == nullptr) return;
    options_menu->open_options();
}

void GameMenu::quit_pressed() {
    if (game != nullptr) {
        game->quit_game();
    }
}

void GameMenu::open_menu() {
    emit_signal("opened");
    show();
}

void GameMenu::close_menu() {
    hide();
    emit_signal("closed");
}

} // namespace ideam::godot_ext