// game_agent_action_sequencer.h
#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgentAction;
class GamePiece;
class SelectionList;

class GameAgentActionSequencer : public godot::VBoxContainer {
    GDCLASS(GameAgentActionSequencer, godot::VBoxContainer)

protected:
    static void _bind_methods();

private:
    // Variables
    GameAgentAction* action = nullptr;

    godot::Label* sequencer_label = nullptr;
    godot::ScrollContainer* scroll = nullptr;
    godot::VBoxContainer* container = nullptr;

    godot::Button* new_step_button = nullptr;

    int max_lines = 5;
    int scroll_lines = 1;
    int line_height = 60;

    godot::Variant undo_redo;
    godot::Variant editor_root;

public:
    GameAgentActionSequencer();
    ~GameAgentActionSequencer();

    // Class Functions
    void set_action_to_sequence(godot::Variant to_sequence);
    
    void _rebuild_sequencer();
    void _deferred_rebuild(); 
    
    void _generate_step(int step_id);
    void _generate_piece_step(int step_id, int piece_step_id, godot::Control* step_container);
    void _generate_piece_action(int step_id, int piece_step_id, int id, godot::Control* piece_container, godot::TypedArray<godot::String> piece_actions);

    void _connect_to_action();
    void _release_action();
    void _clear_sequencer();

    void _update_sequence(godot::String action_name, godot::Array new_sequence);
    void _new_sequence_step();
    void _new_piece_step(int step_id, godot::Container* step_container);
    void _select_piece(int step_id, int piece_step_id, int piece_id);
    void _select_piece_action(int step_id, int piece_step_id, int action_id, int new_selection);
    void _new_action(int step_id, int piece_step_id);

    // Getters/Setters
    void set_undo_redo(godot::Variant p_undo_redo);
    godot::Variant get_undo_redo() const;

    void set_editor_root(godot::Variant p_editor_root);
    godot::Variant get_editor_root() const;
};

} // namespace ideam::godot_ext