#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class ChapterSelect;
class GameOptionsMenu;

class GameMenu : public godot::Control {
    GDCLASS(GameMenu, godot::Control)

protected:
    static void _bind_methods();

private:
    // Exports
    Game* game = nullptr;
    godot::Label* game_title = nullptr;

    godot::Button* new_game_button = nullptr;
    godot::Button* continue_game_button = nullptr;
    godot::Button* load_game_button = nullptr;
    godot::Button* save_game_button = nullptr;
    godot::Button* options_button = nullptr;
    godot::Button* quit_button = nullptr;

    godot::PopupPanel* popup = nullptr;
    ChapterSelect* chapter_select = nullptr;
    GameOptionsMenu* options_menu = nullptr;

public:
    GameMenu();
    ~GameMenu();

    // Godot Functions
    virtual void _ready() override;

    // Setters / Getters
    void set_game(Game* p_game);
    Game* get_game() const;

    void set_game_title(godot::Label* p_label);
    godot::Label* get_game_title() const;

    void set_new_game_button(godot::Button* p_button);
    godot::Button* get_new_game_button() const;

    void set_continue_game_button(godot::Button* p_button);
    godot::Button* get_continue_game_button() const;

    void set_load_game_button(godot::Button* p_button);
    godot::Button* get_load_game_button() const;

    void set_save_game_button(godot::Button* p_button);
    godot::Button* get_save_game_button() const;

    void set_options_button(godot::Button* p_button);
    godot::Button* get_options_button() const;

    void set_quit_button(godot::Button* p_button);
    godot::Button* get_quit_button() const;

    void set_popup(godot::PopupPanel* p_popup);
    godot::PopupPanel* get_popup() const;

    void set_chapter_select(ChapterSelect* p_select);
    ChapterSelect* get_chapter_select() const;

    void set_options_menu(GameOptionsMenu* p_menu);
    GameOptionsMenu* get_options_menu() const;

    // Class Functions
    void validate_game();
    void build_game_menu();

    void validate_new_game_button();
    void validate_continue_game_button();
    void validate_load_game_button();
    void validate_save_game_button();
    void validate_options_button();
    void validate_quit_button();
    void validate_popup();
    void validate_chapter_select();

    void close_chapter_select();
    void new_game_pressed();
    void continue_game_pressed();
    void load_game_pressed();
    void save_game_pressed();
    void options_pressed();
    void quit_pressed();

    void open_menu();
    void close_menu();
};

} // namespace ideam::godot_ext