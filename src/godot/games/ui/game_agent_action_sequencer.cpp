#include "game_agent_action_sequencer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable.hpp>

// Project Header Includes
#include "game_agent_action.h" 
#include "game_piece.h"
// #include "foldable_container.h"
// #include "selection_list.h"

namespace ideam::godot_ext {

void GameAgentActionSequencer::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_action_to_sequence", "to_sequence"), &GameAgentActionSequencer::set_action_to_sequence);
    godot::ClassDB::bind_method(godot::D_METHOD("_rebuild_sequencer"), &GameAgentActionSequencer::_rebuild_sequencer);
    godot::ClassDB::bind_method(godot::D_METHOD("_deferred_rebuild"), &GameAgentActionSequencer::_deferred_rebuild);
    
    godot::ClassDB::bind_method(godot::D_METHOD("_new_sequence_step"), &GameAgentActionSequencer::_new_sequence_step);
    godot::ClassDB::bind_method(godot::D_METHOD("_new_piece_step", "step_id", "step_container"), &GameAgentActionSequencer::_new_piece_step);
    godot::ClassDB::bind_method(godot::D_METHOD("_select_piece", "step_id", "piece_step_id", "piece_id"), &GameAgentActionSequencer::_select_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("_select_piece_action", "step_id", "piece_step_id", "action_id", "new_selection"), &GameAgentActionSequencer::_select_piece_action);
    godot::ClassDB::bind_method(godot::D_METHOD("_new_action", "step_id", "piece_step_id"), &GameAgentActionSequencer::_new_action);

    godot::ClassDB::bind_method(godot::D_METHOD("set_undo_redo", "undo_redo"), &GameAgentActionSequencer::set_undo_redo);
    godot::ClassDB::bind_method(godot::D_METHOD("get_undo_redo"), &GameAgentActionSequencer::get_undo_redo);
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_editor_root", "editor_root"), &GameAgentActionSequencer::set_editor_root);
    godot::ClassDB::bind_method(godot::D_METHOD("get_editor_root"), &GameAgentActionSequencer::get_editor_root);
}

GameAgentActionSequencer::GameAgentActionSequencer() {}
GameAgentActionSequencer::~GameAgentActionSequencer() {}

void GameAgentActionSequencer::set_undo_redo(godot::Variant p_undo_redo) { undo_redo = p_undo_redo; }
godot::Variant GameAgentActionSequencer::get_undo_redo() const { return undo_redo; }

void GameAgentActionSequencer::set_editor_root(godot::Variant p_editor_root) { editor_root = p_editor_root; }
godot::Variant GameAgentActionSequencer::get_editor_root() const { return editor_root; }

void GameAgentActionSequencer::set_action_to_sequence(godot::Variant to_sequence) {
    if (action) {
        _release_action();
    }
    
    action = godot::Object::cast_to<GameAgentAction>(to_sequence);
    
    if (!action) {
        return;
    }
    
    _connect_to_action();
    _rebuild_sequencer();
}

void GameAgentActionSequencer::_rebuild_sequencer() {
    _clear_sequencer();
    
    // Porting the 'await process_frame' logic using call_deferred to mirror the frame wait 
    call_deferred("_deferred_rebuild");
}

void GameAgentActionSequencer::_deferred_rebuild() {
    scroll_lines = 0;
    
    if (!sequencer_label) {
        sequencer_label = memnew(godot::Label);
        sequencer_label->set_text("Sequence");
        add_child(sequencer_label);
    }
    
    if (!scroll) {
        scroll = memnew(godot::ScrollContainer);
        scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        add_child(scroll);
    }
    
    if (!container) {
        container = memnew(godot::VBoxContainer);
        container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        scroll->add_child(container);
    }
    
    if (!new_step_button) {
        new_step_button = memnew(godot::Button);
        new_step_button->set_text("Add Sequence Step");
        new_step_button->connect("pressed", godot::Callable(this, "_new_sequence_step"));
        add_child(new_step_button);
    }
    
    godot::Array sequence = action->call("get_sequence");
    for (int i = 0; i < sequence.size(); ++i) {
        if (scroll_lines < max_lines) {
            scroll_lines += 1;
        }
        _generate_step(i);
    }
    
    if (sequence.size() > 1) {
        scroll->set_custom_minimum_size(godot::Vector2i(0, scroll_lines * line_height));
    } else {
        scroll->set_custom_minimum_size(godot::Vector2i(0, line_height));
    }
}

void GameAgentActionSequencer::_generate_step(int step_id) {
    godot::Array sequence = action->call("get_sequence");
    godot::Array step = sequence[step_id];
    
    // Assuming FoldableContainer is a registered type
    godot::Control* step_drop = godot::Object::cast_to<godot::Control>(godot::ClassDB::instantiate("FoldableContainer"));
    container->add_child(step_drop);
    step_drop->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    step_drop->call("fold");
    step_drop->set("title", "Step " + godot::String::num_int64(step_id + 1));
    
    godot::VBoxContainer* step_container = memnew(godot::VBoxContainer);
    step_drop->add_child(step_container);
    
    godot::Button* new_piece_button = memnew(godot::Button);
    new_piece_button->set_text("Add Piece Step");
    
    // Handle lambda for adding new piece [cite: 28]
    new_piece_button->connect("pressed", godot::Callable(this, "_new_piece_step").bind(step_id, step_container));
    
    for (int i = 1; i < step.size(); ++i) {
        _generate_piece_step(step_id, i, step_container);
    }
    
    step_container->add_child(new_piece_button);
    step_container->hide();
}

void GameAgentActionSequencer::_generate_piece_step(int step_id, int piece_step_id, godot::Control* step_container) {
    godot::Array sequence = action->call("get_sequence");
    godot::Array step = sequence[step_id];
    godot::Array piece_step = step[piece_step_id];
    
    int piece_id = piece_step[0];
    GamePiece* piece = nullptr;

    godot::Array game_pieces = action->call("get_game_pieces");
    if (piece_id >= 0 && piece_id < game_pieces.size()) {
        piece = godot::Object::cast_to<GamePiece>(game_pieces[piece_id]);
    }
    
    godot::Control* piece_panel_container = godot::Object::cast_to<godot::Control>(godot::ClassDB::instantiate("FoldableContainer"));
    piece_panel_container->set("title", "Game Piece: ");
    step_container->add_child(piece_panel_container);
    piece_panel_container->call("fold");
    
    godot::Control* piece_selector = godot::Object::cast_to<godot::Control>(godot::ClassDB::instantiate("Selection_List"));
    piece_panel_container->call("add_title_bar_control", piece_selector);
    piece_selector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    
    godot::ScrollContainer* piece_scroll = memnew(godot::ScrollContainer);
    piece_panel_container->add_child(piece_scroll);
    
    godot::VBoxContainer* piece_container = memnew(godot::VBoxContainer);
    piece_scroll->add_child(piece_container);
    
    godot::Button* new_action_button = memnew(godot::Button);
    new_action_button->set_text("Add Piece Action");
    piece_container->add_child(new_action_button);
    
    new_action_button->connect("pressed", godot::Callable(this, "_new_action").bind(step_id, piece_step_id));
    
    godot::Array piece_names;
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* p = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (p) piece_names.append(p->call("get_name"));
    }
    
    piece_selector->call("build_list", piece_names);
    piece_selector->call("set_selection", piece_id);
    
    // Handle lambda for piece selection with the "Same Piece!" print [cite: 29]
    piece_selector->connect("selection_changed", godot::Callable(this, "_select_piece").bind(step_id, piece_step_id));
    
    godot::Array piece_actions;
    if (piece) {
        godot::Array actions = piece->call("get_actions");
        for (int j = 0; j < actions.size(); ++j) {
            godot::Object* act = actions[j];
            if (act) piece_actions.append(act->call("get_name"));
        }
    }
    
    for (int j = 1; j < piece_step.size(); ++j) {
        _generate_piece_action(step_id, piece_step_id, j, piece_container, piece_actions);
    }
}

void GameAgentActionSequencer::_generate_piece_action(int step_id, int piece_step_id, int id, godot::Control* piece_container, godot::Array piece_actions) {
    godot::Array sequence = action->call("get_sequence");
    godot::Array step = sequence[step_id];
    godot::Array piece_step = step[piece_step_id];
    int action_id = piece_step[id];
    
    godot::Control* action_selector = godot::Object::cast_to<godot::Control>(godot::ClassDB::instantiate("Selection_List"));
    action_selector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    piece_container->add_child(action_selector);
    
    action_selector->call("build_list", piece_actions);
    action_selector->call("set_selection", action_id);
    
    action_selector->connect("selection_changed", godot::Callable(this, "_select_piece_action").bind(step_id, piece_step_id, id));
}

void GameAgentActionSequencer::_connect_to_action() {
    if (!action) return;
}

void GameAgentActionSequencer::_release_action() {
    if (!action) return;
    action = nullptr;
}

void GameAgentActionSequencer::_clear_sequencer() {
    if (container) {
        container->queue_free();
        container = nullptr;
    }
}

void GameAgentActionSequencer::_update_sequence(godot::String action_name, godot::Array new_sequence) {
    if (undo_redo.get_type() != godot::Variant::NIL) {
        undo_redo.call("create_action", action_name);
        godot::Array old_sequence = action->call("get_sequence").duplicate(true);
        
        undo_redo.call("add_do_property", action, "sequence", new_sequence);
        undo_redo.call("add_undo_property", action, "sequence", old_sequence);
        undo_redo.call("commit_action");
    } else {
        action->set("sequence", new_sequence);
    }
    
    _rebuild_sequencer();
}

void GameAgentActionSequencer::_new_sequence_step() {
    godot::Array new_step;
    godot::Array act_seq = action->call("get_sequence").duplicate(true);
    act_seq.append(new_step);
    
    if (undo_redo.get_type() == godot::Variant::NIL) {
        action->set("sequence", act_seq);
        _rebuild_sequencer();
        return;
    }
    
    _update_sequence("New Sequence Step", act_seq);
}

void GameAgentActionSequencer::_new_piece_step(int step_id, godot::Container* step_container) {
    godot::Array sequence = action->call("get_sequence");
    godot::Array step = sequence[step_id].duplicate(true);
    godot::Array new_piece_val;
    new_piece_val.append(-1);
    step.append(new_piece_val);
    
    godot::Array act_seq = action->call("get_sequence").duplicate(true);
    act_seq[step_id] = step;

    if (undo_redo.get_type() == godot::Variant::NIL) {
        action->set("sequence", act_seq);
        _rebuild_sequencer();
        return;
    }
    
    _update_sequence("New Piece Step", act_seq);
}

void GameAgentActionSequencer::_select_piece(int step_id, int piece_step_id, int piece_id) {
    godot::Array sequence = action->call("get_sequence");
    godot::Array step = sequence[step_id].duplicate();
    godot::Array piece_step = step[piece_step_id].duplicate();
    
    if (int(piece_step[0]) == piece_id) {
        godot::UtilityFunctions::print("Same Piece!");
        return;
    }
    
    piece_step[0] = piece_id;
    step[piece_step_id] = piece_step;
    
    godot::Array act_seq = action->call("get_sequence").duplicate(true);
    act_seq[step_id] = step;
    
    if (undo_redo.get_type() == godot::Variant::NIL) {
        action->set("sequence", act_seq);
        _rebuild_sequencer();
        return;
    }
    
    _update_sequence("Select Piece", act_seq);
}

void GameAgentActionSequencer::_select_piece_action(int step_id, int piece_step_id, int action_id, int new_selection) {
    godot::Array sequence = action->call("get_sequence");
    godot::Array step = sequence[step_id].duplicate(true);
    godot::Array piece_step = step[piece_step_id];

    int old_selection = piece_step[action_id];
    if (old_selection == new_selection) {
        return;
    }
    
    piece_step[action_id] = new_selection;
    step[piece_step_id] = piece_step;
    
    godot::Array act_seq = action->call("get_sequence").duplicate(true);
    act_seq[step_id] = step;
    
    if (undo_redo.get_type() == godot::Variant::NIL) {
        action->set("sequence", act_seq);
        _rebuild_sequencer();
        return;
    }
    
    _update_sequence("Select Action", act_seq);
}

void GameAgentActionSequencer::_new_action(int step_id, int piece_step_id) {
    godot::Array new_sequence = action->call("get_sequence").duplicate(true);
    godot::Array step = new_sequence[step_id];
    godot::Array piece_step = step[piece_step_id];
    
    piece_step.append(-1);
    step[piece_step_id] = piece_step;
    new_sequence[step_id] = step;
    
    if (undo_redo.get_type() == godot::Variant::NIL) {
        action->set("sequence", new_sequence);
        _rebuild_sequencer();
        return;
    }
    
    _update_sequence("New Piece Step Action", new_sequence);
}

} // namespace ideam::godot_ext