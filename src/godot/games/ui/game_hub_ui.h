// game_hub_ui.h
#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../game_hub.h"
#include "../game.h"

namespace ideam::godot_ext {

class GameHubUI : public godot::Control {
    GDCLASS(GameHubUI, godot::Control)

protected:
    static void _bind_methods();

private:
    
    GameHub* game_hub = nullptr;
    godot::ItemList* games_list = nullptr;
    godot::Control* selected_game_field = nullptr;
    godot::Label* title_label = nullptr;
    godot::Label* path_label = nullptr;
    godot::Label* game_state_label = nullptr;
    godot::Button* game_action_button = nullptr;

    bool initialized = false;
    int selected_game_id = -1;
    int action_id = -1;


public:
    GameHubUI();
    ~GameHubUI();

    virtual void _ready() override;

    // Getters / Setters
    void set_game_hub(GameHub* p_hub);
    GameHub* get_game_hub() const;

    void set_games_list(godot::ItemList* p_list);
    godot::ItemList* get_games_list() const;

    void set_selected_game_field(godot::Control* p_field);
    godot::Control* get_selected_game_field() const;

    void set_title_label(godot::Label* p_label);
    godot::Label* get_title_label() const;

    void set_path_label(godot::Label* p_label);
    godot::Label* get_path_label() const;

    void set_game_state_label(godot::Label* p_label);
    godot::Label* get_game_state_label() const;

    void set_game_action_button(godot::Button* p_button);
    godot::Button* get_game_action_button() const;

    void find_game_hub();
    void validate_games_list();
    void validate_selected_game_field();
    void validate_title_label();
    void validate_path_label();
    void validate_game_state_label();
    void validate_game_action_button();
    
    void build_games_list();
    void game_selected(int game_id);
    void game_action();
    void get_game_state();

};

} // namespace ideam::godot_ext