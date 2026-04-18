#pragma once

#include "../../game_entity.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GamePiece;
class GameInteraction;

// DOD NOTE: The `GameAgentAction` fundamentally operates as a sequence manager. 
// Using a jagged `Array` of `Array`s for `sequence` allocates a scattershot memory 
// layout across the heap, guaranteeing cache misses on every `sequence_step()`. 
// In a high-performance C++26 Data-Oriented approach, `sequence` should be flattened 
// into a single contiguous `std::vector<SequenceStepPOD>` where a struct holds 
// `uint32_t piece_id` and `uint32_t action_id`.
class GameAgentAction : public GameEntity {
    GDCLASS(GameAgentAction, GameEntity)

protected:
    static void _bind_methods();

private:
    godot::TypedArray<GamePiece> game_pieces;
    int priority = 0;
    
    // DOD NOTE: `competes_with_groups` represents string-based tags. String comparisons 
    // are detrimental in hot loops. Convert this to a single 64-bit `uint64_t competition_mask` 
    // where each bit represents a group, allowing O(1) bitwise collision checks.
    godot::TypedArray<godot::String> competes_with_groups;
    godot::Array sequence; // Array of Arrays

    int current_sequence_id = -1;
    bool in_progress = false;
    int current_value = 0;
    
    godot::TypedArray<GamePiece> sequence_targets;

public:
    GameAgentAction();
    ~GameAgentAction();

    // Setters / Getters
    void set_game_pieces(const godot::TypedArray<GamePiece>& p_pieces);
    godot::TypedArray<GamePiece> get_game_pieces() const;

    void set_priority(int p_priority);
    int get_priority() const;

    void set_competes_with_groups(const godot::TypedArray<godot::String>& p_groups);
    godot::TypedArray<godot::String> get_competes_with_groups() const;

    void set_sequence(const godot::Array& p_sequence);
    godot::Array get_sequence() const;

    int get_current_sequence_id() const;
    bool get_in_progress() const;

    // Class Functions
    int get_current_value();

    // Action Functions
    void start_action(int target_id = -1);
    void start_targeted_action(GamePiece* target);

    // Sequence Functions
    void add_sequence_step(const godot::Array& new_sequence_step);
    void remove_sequence_step_at(int id);
    void new_piece_step(int sequence_step);
    void add_piece_step(int sequence_step, const godot::Array& new_step);
    void remove_piece_step_at(int sequence_step, int id);
    void target_for_sequence(GamePiece* target);
    void sequence_step_forward();
    void sequence_step_backward();
    void sequence_step(int id);

    void end_action();
    void interrupt_action();
    void fail_action(int margin_of_failure);
    void complete_action(int margin_of_victory);
    
    // GameEntity Overrides
    virtual void action_consequences(int score, const godot::Dictionary& consequences) override;
    
    void apply_action_value();
    void _finalize_current_step();

    // Game Piece Functions
    godot::TypedArray<godot::String> gather_game_piece_titles() const;
    int add_game_piece(GamePiece* new_game_piece);
    int get_game_piece_id(GamePiece* game_piece) const;
    bool has_game_piece(GamePiece* game_piece) const;
    bool remove_game_piece(GamePiece* game_piece);
    bool remove_game_piece_at(int game_piece_ID);

    // Function Overrides
    virtual void game_process(double delta) override;
    virtual void game_process_clear() override;
};

} // namespace ideam::godot_ext