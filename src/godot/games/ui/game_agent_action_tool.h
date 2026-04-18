#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgent;
class GameAgentAction;
class GameAgentActionSequencer;
class GameAgent_EditorInspectorPlugin;
class GamePiece;
class GamePieceAction;
class FoldableContainer;

class GameAgentActionTool : public godot::VBoxContainer {
    GDCLASS(GameAgentActionTool, godot::VBoxContainer)

protected:
    static void _bind_methods();

private:
    // Exports
    godot::LineEdit* name_entry = nullptr;
    godot::Button* new_piece_button = nullptr;
    godot::Container* game_pieces_list = nullptr;
    godot::ScrollContainer* game_pieces_scroll = nullptr;
    GameAgentActionSequencer* action_sequencer = nullptr;
    godot::Button* confirm_button = nullptr;
    godot::Button* cancel_button = nullptr;

    // Variables
    int line_size = 45;
    GameAgentAction* _new_action = nullptr;
    godot::Array _new_pieces;
    GameAgent* _agent = nullptr;
    GameAgent_EditorInspectorPlugin* _editor = nullptr;

public:
    GameAgentActionTool();
    ~GameAgentActionTool();

    // Setters / Getters for Exports
    void set_name_entry(godot::LineEdit* p_entry);
    godot::LineEdit* get_name_entry() const;

    void set_new_piece_button(godot::Button* p_button);
    godot::Button* get_new_piece_button() const;

    void set_game_pieces_list(godot::Container* p_list);
    godot::Container* get_game_pieces_list() const;

    void set_game_pieces_scroll(godot::ScrollContainer* p_scroll);
    godot::ScrollContainer* get_game_pieces_scroll() const;

    void set_action_sequencer(GameAgentActionSequencer* p_sequencer);
    GameAgentActionSequencer* get_action_sequencer() const;

    void set_confirm_button(godot::Button* p_button);
    godot::Button* get_confirm_button() const;

    void set_cancel_button(godot::Button* p_button);
    godot::Button* get_cancel_button() const;

    // Class Functions
    void open_tool(godot::Object* agent, godot::Object* editor = nullptr);
    void new_game_piece();
    void confirm_and_create();
    void cancel_and_close();
    void close_tool();
    
    godot::Array _add_piece_entry(godot::Object* game_piece = nullptr);
    bool _check_name();
    bool _check_new_pieces();
    void _get_min_y_size();

    // Internal Helper for Lambda Connections
    void _add_piece_action_callback(godot::VBoxContainer* piece_actions_list);
};

} // namespace ideam::godot_ext