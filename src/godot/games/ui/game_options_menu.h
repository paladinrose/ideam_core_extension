#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameMenu;

class GameOptionsMenu : public godot::Control {
    GDCLASS(GameOptionsMenu, godot::Control)

protected:
    static void _bind_methods();

private:
    // Exports
    Game* game = nullptr;
    GameMenu* game_menu = nullptr;
    godot::Button* apply_button = nullptr;
    godot::Button* cancel_button = nullptr;

public:
    GameOptionsMenu();
    ~GameOptionsMenu();

    // Setters / Getters
    void set_game(Game* p_game);
    Game* get_game() const;

    void set_game_menu(GameMenu* p_menu);
    GameMenu* get_game_menu() const;

    void set_apply_button(godot::Button* p_button);
    godot::Button* get_apply_button() const;

    void set_cancel_button(godot::Button* p_button);
    godot::Button* get_cancel_button() const;

    // Class Functions
    void open_options();
    void close_options();
    void apply_options();
    void cancel_options();
};

} // namespace ideam::godot_ext