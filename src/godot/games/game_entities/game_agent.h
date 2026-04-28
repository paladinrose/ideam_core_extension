#pragma once

#include "../game_entity.h" 
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GamePlayer;
class GameAgentAction;
class GamePiece;
class NodeRetargeter;
class SignalConnector;
class GameInteraction;
class Game;

// DOD NOTE: Extracting these states from the object header allows for dense packing 
// in an ECS-like system. Instead of traversing a hierarchy of Nodes to check exclusivity, 
// a central manager can evaluate a flat `std::span<AgentExclusivity>` for all agents in cache.
enum AgentExclusivity : int32_t {
    NONE = 0,
    EXCLUSIVE = 1,
    GROUP_INCLUSIVE = 2,
    GROUP_EXCLUSIVE = 3
};

enum AgentMatching : int32_t {
    MATCH_NONE = 0, 
    PLAYER_TO_AGENT = 1,
    AGENT_TO_PLAYER = 2,
    AVERAGE = 3
};

class GameAgent : public GameEntity {
    GDCLASS(GameAgent, GameEntity)

protected:
    static void _bind_methods();

private:
    GamePlayer* _player = nullptr;
    godot::Node* player_parent_target = nullptr;
    
    AgentExclusivity exclusivity = AgentExclusivity::EXCLUSIVE;
    
    AgentMatching match_position = AgentMatching::PLAYER_TO_AGENT;
    float player_reposition_time = 0.1f;
    
    AgentMatching match_rotation = AgentMatching::PLAYER_TO_AGENT;
    float player_reorient_time = 0.1f;
    
    AgentMatching match_scale = AgentMatching::PLAYER_TO_AGENT;
    float player_rescale_time = 0.1f;

    // DOD NOTE: `TypedArray` encapsulates Godot Variants, causing double-indirection 
    // when iterating. In a data-oriented architecture, Actions and GamePieces should be 
    // allocated sequentially in memory using `std::vector<GameAgentAction*>` or 
    // even better, by storing pure data structures and generating handles.
    godot::TypedArray<GameAgentAction> actions;
    godot::TypedArray<GamePiece> game_pieces;

    NodeRetargeter* player_node_assignments = nullptr;
    SignalConnector* player_signal_assignments = nullptr;

    godot::TypedArray<GameInteraction> interactions;
    godot::TypedArray<GameEntity> current_action_targets;

    GamePlayer* _started_to_control = nullptr;
    GamePlayer* _waiting_to_control = nullptr;

    bool _position_match_complete = false;
    bool _rotation_match_complete = false;
    bool _scale_match_complete = false;

public:
    GameAgent();
    ~GameAgent();

    // Setters / Getters
    void set_player(GamePlayer* p_player);
    GamePlayer* get_player() const;

    void set_player_parent_target(godot::Node* p_target);
    godot::Node* get_player_parent_target() const;

    void set_exclusivity(AgentExclusivity p_exclusivity);
    AgentExclusivity get_exclusivity() const;

    void set_match_position(AgentMatching p_match);
    AgentMatching get_match_position() const;

    void set_player_reposition_time(float p_time);
    float get_player_reposition_time() const;

    void set_match_rotation(AgentMatching p_match);
    AgentMatching get_match_rotation() const;

    void set_player_reorient_time(float p_time);
    float get_player_reorient_time() const;

    void set_match_scale(AgentMatching p_match);
    AgentMatching get_match_scale() const;

    void set_player_rescale_time(float p_time);
    float get_player_rescale_time() const;

    void set_actions(const godot::TypedArray<GameAgentAction>& p_actions);
    godot::TypedArray<GameAgentAction> get_actions() const;

    void set_game_pieces(const godot::TypedArray<GamePiece>& p_pieces);
    godot::TypedArray<GamePiece> get_game_pieces() const;

    void set_player_node_assignments(NodeRetargeter* p_assignments);
    NodeRetargeter* get_player_node_assignments() const;

    void set_player_signal_assignments(SignalConnector* p_assignments);
    SignalConnector* get_player_signal_assignments() const;

    // Player Functions
    void find_and_possess_agent(godot::Variant on);
    void find_and_swap_agent(godot::Variant on);
    void start_control(GamePlayer* new_player);
    void continue_control();
    void control(GamePlayer* new_player);
    void release_and_control(GamePlayer* to_control);
    void start_release();
    void control_end();

    // Matching Functions
    void start_position_match(GamePlayer* player_to_match = nullptr);
    void _position_match(float percent);
    void complete_position_match();

    void start_rotation_match(GamePlayer* player_to_match = nullptr);
    void _rotation_match(float percent);
    void complete_rotation_match();

    void start_scale_match(GamePlayer* player_to_match = nullptr);
    void _scale_match(float percent);
    void complete_scale_match();

    void _check_position_match();
    void _check_rotation_match();
    void _check_scale_match();
    void _check_all_matches();

    // Action Functions
    godot::TypedArray<godot::String> gather_action_titles() const;
    int add_action(GameAgentAction* new_action);
    int get_action_id_from_title(const godot::String& title) const;
    int get_action_id(GameAgentAction* action) const;
    bool has_action(GameAgentAction* action) const;
    bool remove_action(GameAgentAction* action);
    bool remove_action_at(int action_ID);
    
    godot::Array find_best_action(GameAgentAction* instigating_action) const; 
    
    void start_action(int action_id, int target_id = -1);
    void targeted_action(int action_id, GamePiece* target);
    void end_action(int action_id);
    void interrupt_action(int action_id);
    void fail_action(int action_id, int margin_of_failure);
    void complete_action(int action_id, int margin_of_victory);
    int get_action_value(int action_id) const;
    void apply_action_value_property(const godot::NodePath& to, int action_id, const godot::String& property_path);
    void apply_action_value_method(const godot::NodePath& to, int action_id, const godot::String& method_name);
    
    GameInteraction* interact(int action_id);
    void join_interaction(GameInteraction* interaction);
    void leave_interaction(GameInteraction* interaction);
    void stop_interacting(int action_id);

    // Consequences
    void action_consequences(int score, const godot::Dictionary& consequences);

    // Game Piece Functions
    godot::TypedArray<godot::String> gather_game_piece_titles() const;
    int add_game_piece(GamePiece* new_game_piece);
    int get_game_piece_id(GamePiece* game_piece) const;
    bool has_game_piece(GamePiece* game_piece) const;
    bool remove_game_piece(GamePiece* game_piece);
    bool remove_game_piece_at(int game_piece_ID);
    void handle_agent_request(GamePiece* game_piece);

    // GameEntity Overrides
    virtual void enter_game() override;
    virtual void game_start() override;
    virtual void game_pause() override;
    virtual void game_continue() override;
    virtual void game_end() override;
    virtual void exit_game() override;
    virtual void game_process(double delta) override;
    virtual void game_process_clear() override;
    virtual godot::Dictionary save_data() const override;
    virtual void load_data(const godot::Dictionary& data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::AgentExclusivity);
VARIANT_ENUM_CAST(ideam::godot_ext::AgentMatching);