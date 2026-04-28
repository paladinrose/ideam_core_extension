// chapter_select.h
#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class Game;
class GameBoard;

class ChapterSelect : public godot::Control {
    GDCLASS(ChapterSelect, godot::Control)

protected:
    static void _bind_methods();

private:
    godot::ItemList* chapters_list = nullptr;
    godot::Button* load_chapter_button = nullptr;
    godot::Button* cancel_button = nullptr;
    godot::Label* message_label = nullptr;

    Game* game = nullptr;
    int selected_chapter = -1;

public:
    ChapterSelect();
    ~ChapterSelect();

    virtual void _ready() override;

    void set_chapters_list(godot::ItemList* p_list);
    godot::ItemList* get_chapters_list() const;

    void set_load_chapter_button(godot::Button* p_button);
    godot::Button* get_load_chapter_button() const;

    void set_cancel_button(godot::Button* p_button);
    godot::Button* get_cancel_button() const;

    void set_message_label(godot::Label* p_label);
    godot::Label* get_message_label() const;

    void validate_game();
    void build_chapter_selector();
    void select_chapter(int chapterID);
    void load_chapter();
    void cancel_chapter_select();
    godot::String get_game_board_label(int game_id);
    
    void game_board_already_loaded(Game* p_game, GameBoard* p_game_board);
    void game_board_not_found(Game* p_game, int game_board_id);
    void game_board_load_start(Game* p_game, int game_board_id);
    void game_board_load_complete(Game* p_game, GameBoard* p_game_board);
};

} // namespace ideam::godot_ext