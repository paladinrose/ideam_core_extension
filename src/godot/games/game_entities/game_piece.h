#pragma once

#include "../game_entity.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameProperty;
class GamePieceAction;
class GameAgent;
class GameInteraction;

// DOD NOTE: GamePiece represents a hierarchical structure of Nodes (Sub-Pieces, Actions, Properties).
// This nested Node hierarchy completely destroys spatial locality. Traversing this tree during 
// recursive calls like `get_action_value` requires the CPU to jump across disparate memory pages. 
// For high-performance architectures, recursive tree-chasing should be replaced by flat arrays 
// mapping action IDs directly to cached aggregate values updated via an event queue.
class GamePiece : public GameEntity {
    GDCLASS(GamePiece, GameEntity)

protected:
    static void _bind_methods();

private:
    godot::TypedArray<GameProperty> game_properties;
    godot::TypedArray<GamePieceAction> actions;
    godot::TypedArray<GamePiece> sub_game_pieces;

    int max_sub_pieces = 0;
    godot::TypedArray<GameAgent> requested_agents;

public:
    GamePiece();
    ~GamePiece();

    virtual void _ready() override;

    // Setters / Getters
    void set_game_properties(const godot::TypedArray<GameProperty>& p_props);
    godot::TypedArray<GameProperty> get_game_properties() const;

    void set_actions(const godot::TypedArray<GamePieceAction>& p_actions);
    godot::TypedArray<GamePieceAction> get_actions() const;

    void set_sub_game_pieces(const godot::TypedArray<GamePiece>& p_sub_pieces);
    godot::TypedArray<GamePiece> get_sub_game_pieces() const;

    void set_max_sub_pieces(int p_max);
    int get_max_sub_pieces() const;

    // Use Functions
    void collect_actions();
    void clear_actions();
    int add_action(GamePieceAction* new_action);
    int get_action_id_from_title(const godot::String& title) const;
    int get_action_id(GamePieceAction* action) const;
    bool has_action(GamePieceAction* action) const;
    bool remove_action(GamePieceAction* action);
    bool remove_action_at(int action_ID);
    
    godot::Array find_best_action(GamePieceAction* instigating_action);
    void take_action(int action_id);
    void stop_acting(int action_id);
    int get_action_value(int action_id);
    
    GameInteraction* interact(int action_id);
    void join_interaction(GameInteraction* interaction);

    // Property Functions
    void collect_properties();
    void clear_properties();
    int add_property(GameProperty* property);
    bool has_property_name(const godot::String& property_name) const;
    GameProperty* get_property(const godot::String& property_name) const;
    int get_property_id(const godot::String& property_name) const;
    void exhaust_property(GameProperty* game_property);
    bool remove_property(GameProperty* game_property);
    bool remove_property_at(int id);

    // Sub-Piece Functions
    void find_sub_piece(godot::Variant on);
    int add_sub_piece(GamePiece* sub_piece);
    int get_sub_piece_id(const GamePiece* sub_piece) const;
    bool is_in_sub_piece_chain(const GamePiece* sub_piece) const;
    bool can_accept_sub_piece(const GamePiece* sub) const;
    bool remove_sub_piece(GamePiece* sub_piece);
    bool remove_sub_piece_at(int id);
    void find_and_remove_sub_piece(godot::Variant on);
    godot::TypedArray<GameAgent> request_agents();

    // GameEntity Overrides
    virtual void enter_game() override;
    virtual void game_pause() override;
    virtual void game_continue() override;
    virtual void exit_game() override;
    virtual void game_process(double delta) override;
    virtual void game_process_clear() override;
    virtual void action_consequences(int score, const godot::Dictionary& consequences) override;
    
    virtual godot::Dictionary save_data() const override;
    virtual void load_data(const godot::Dictionary& data) override;
};

} // namespace ideam::godot_ext