#include "game_options_menu.h"
#include <godot_cpp/core/class_db.hpp>

// Assuming these project headers exist
#include "../game.h"
#include "game_menu.h"

namespace ideam::godot_ext {

void GameOptionsMenu::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("applied"));
    ADD_SIGNAL(godot::MethodInfo("canceled"));
    ADD_SIGNAL(godot::MethodInfo("opened"));
    ADD_SIGNAL(godot::MethodInfo("closed"));

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("open_options"), &GameOptionsMenu::open_options);
    godot::ClassDB::bind_method(godot::D_METHOD("close_options"), &GameOptionsMenu::close_options);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_options"), &GameOptionsMenu::apply_options);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_options"), &GameOptionsMenu::cancel_options);

    // Property Bindings
    godot::ClassDB::bind_method(godot::D_METHOD("set_game", "game"), &GameOptionsMenu::set_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game"), &GameOptionsMenu::get_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game"), "set_game", "get_game");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_menu", "game_menu"), &GameOptionsMenu::set_game_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_menu"), &GameOptionsMenu::get_game_menu);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_menu", godot::PROPERTY_HINT_NODE_TYPE, "GameMenu"), "set_game_menu", "get_game_menu");

    godot::ClassDB::bind_method(godot::D_METHOD("set_apply_button", "apply_button"), &GameOptionsMenu::set_apply_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_apply_button"), &GameOptionsMenu::get_apply_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "apply_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_apply_button", "get_apply_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cancel_button", "cancel_button"), &GameOptionsMenu::set_cancel_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cancel_button"), &GameOptionsMenu::get_cancel_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cancel_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_cancel_button", "get_cancel_button");
}

GameOptionsMenu::GameOptionsMenu() {}
GameOptionsMenu::~GameOptionsMenu() {}

// Setters / Getters
void GameOptionsMenu::set_game(Game* p_game) { game = p_game; }
Game* GameOptionsMenu::get_game() const { return game; }

void GameOptionsMenu::set_game_menu(GameMenu* p_menu) { game_menu = p_menu; }
GameMenu* GameOptionsMenu::get_game_menu() const { return game_menu; }

void GameOptionsMenu::set_apply_button(godot::Button* p_button) { apply_button = p_button; }
godot::Button* GameOptionsMenu::get_apply_button() const { return apply_button; }

void GameOptionsMenu::set_cancel_button(godot::Button* p_button) { cancel_button = p_button; }
godot::Button* GameOptionsMenu::get_cancel_button() const { return cancel_button; }

// Class Functions
// Class Functions
void GameOptionsMenu::open_options() {
    this->show();
    emit_signal("opened");
}

void GameOptionsMenu::close_options() {
    this->hide();
    emit_signal("closed");
}

void GameOptionsMenu::apply_options() {
    emit_signal("applied");
    close_options();
}

void GameOptionsMenu::cancel_options() {
    emit_signal("canceled");
    close_options();
}

} // namespace ideam::godot_ext