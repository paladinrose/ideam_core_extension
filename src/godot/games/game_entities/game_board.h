#pragma once

#include "../game_entity.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameAgent;
class GamePiece;

// DOD NOTE: GameBoard currently acts as an Array of Structures (AoS) manager, 
// holding arrays of object pointers (`GameAgent*`, `GamePiece*`). 
// For optimal CPU cache utilization, GameBoard should ideally be refactored into a 
// central Systems Coordinator that owns dense Structure of Arrays (SoA) data 
// (e.g., `std::vector<Position>`, `std::vector<Health>`), while Agents and Pieces 
// become mere 32-bit integer IDs indexing into these arrays.
class GameBoard : public GameEntity {
    GDCLASS(GameBoard, GameEntity)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: `TypedArray` enforces variant wrapping and heap indirection. 
    // In high-performance C++ codepaths, replace these with `std::vector<GameAgent*>` 
    // or flat memory pools. Iterating a `TypedArray` during `_process` forces 
    // the CPU prefetcher to stall constantly.
    godot::TypedArray<GameAgent> game_agents;
    godot::TypedArray<GamePiece> game_pieces;

public:
    GameBoard();
    ~GameBoard();

    // Setters / Getters
    void set_game_agents(const godot::TypedArray<GameAgent>& p_agents);
    godot::TypedArray<GameAgent> get_game_agents() const;

    void set_game_pieces(const godot::TypedArray<GamePiece>& p_pieces);
    godot::TypedArray<GamePiece> get_game_pieces() const;

    // Class Functions
    void request_quit_game();
    void request_pause_game();
    void reset_game_board();

    // Game Agent Functions
    // DOD NOTE: Extracting string titles every frame or on-demand causes severe 
    // string allocation overhead. String hashing (`godot::StringName` or `uint32_t` hashes) 
    // should be preferred for metadata lookups.
    godot::TypedArray<godot::String> gather_game_agent_titles() const;
    int add_game_agent(GameAgent* new_game_agent);
    int get_game_agent_id(GameAgent* game_agent) const;
    bool has_game_agent(GameAgent* game_agent) const;
    bool remove_game_agent(GameAgent* game_agent);
    bool remove_game_agent_at(int game_agent_ID);

    // Game Piece Functions
    godot::TypedArray<godot::String> gather_game_piece_titles() const;
    int add_game_piece(GamePiece* new_game_piece);
    int get_game_piece_id(GamePiece* game_piece) const;
    bool has_game_piece(GamePiece* game_piece) const;
    bool remove_game_piece(GamePiece* game_piece);
    bool remove_game_piece_at(int game_piece_ID);

    godot::TypedArray<GameAgent> get_controlling_agents(GamePiece* game_piece) const;

    // GameEntity Overrides
    virtual void enter_game() override;
    virtual void game_start() override;
    virtual void game_pause() override;
    virtual void game_continue() override;
    virtual void game_end() override;
    virtual void exit_game() override;
    
    // DOD NOTE: This `game_process` implementation relies on virtual dispatches 
    // (`game_process()` calls) across heterogeneous objects scattered in memory. 
    // Replacing this with a Job System that batches continuous updates over 
    // contiguous data sequences will heavily parallelize operations and saturate SIMD lanes.
    virtual void game_process(double delta) override;
    virtual void game_process_clear() override;
    
    virtual godot::Dictionary save_data() const override;
    virtual void load_data(const godot::Dictionary& data) override;
};

} // namespace ideam::godot_ext