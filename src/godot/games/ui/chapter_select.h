#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameBoard;

class ChapterSelect : public godot::Control {
    GDCLASS(ChapterSelect, godot::Control)

protected:
    static void _bind_methods();

private:
    // Exports
    godot::ItemList* chapters_list = nullptr;
    godot::Button* load_chapter_button = nullptr;
    godot::Button* cancel_button = nullptr;
    godot::Label* message_label = nullptr;

    // Variables
    Game* game = nullptr;
    int selected_chapter = -1;

public:
    ChapterSelect();
    ~ChapterSelect();

    // Godot Functions
    virtual void _ready() override;

    // Setters and Getters for Exports
    void set_chapters_list(godot::ItemList* p_list);
    godot::ItemList* get_chapters_list() const;

    void set_load_chapter_button(godot::Button* p_button);
    godot::Button* get_load_chapter_button() const;

    void set_cancel_button(godot::Button* p_button);
    godot::Button* get_cancel_button() const;

    void set_message_label(godot::Label* p_label);
    godot::Label* get_message_label() const;

    // Class Functions
    void validate_game();
    void build_chapter_selector();
    void select_chapter(int chapterID);
    void load_chapter();
    void cancel_chapter_select();
    godot::String get_game_board_label(int game_id);
    
    void game_board_already_loaded(godot::Object* p_game, godot::Object* p_game_board);
    void game_board_not_found(godot::Object* p_game, int game_board_id);
    void game_board_load_start(godot::Object* p_game, int game_board_id);
    void game_board_load_complete(godot::Object* p_game, godot::Object* p_game_board);
};

} // namespace ideam::godot_ext