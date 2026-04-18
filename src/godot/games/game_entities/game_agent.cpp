#include <godot_cpp/core/class_db.hpp>
#include "game_agent.h"
#include "../game_player.h"
#include "actions/game_agent_action.h"
#include "game_piece.h"
// #include "node_retargeter.h"
// #include "signal_connector.h"
#include "actions/game_interaction.h"

namespace ideam::godot_ext {

void GameAgent::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("action_added", godot::PropertyInfo(godot::Variant::OBJECT, "action", godot::PROPERTY_HINT_NODE_TYPE, "GameAgentAction")));
    ADD_SIGNAL(godot::MethodInfo("action_removed", godot::PropertyInfo(godot::Variant::OBJECT, "action", godot::PROPERTY_HINT_NODE_TYPE, "GameAgentAction")));
    ADD_SIGNAL(godot::MethodInfo("game_piece_added", godot::PropertyInfo(godot::Variant::OBJECT, "new_piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    ADD_SIGNAL(godot::MethodInfo("game_piece_removed", godot::PropertyInfo(godot::Variant::OBJECT, "removed", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    
    ADD_SIGNAL(godot::MethodInfo("player_control_started", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_control_release_started", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_released", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));

    ADD_SIGNAL(godot::MethodInfo("match_position_started"));
    ADD_SIGNAL(godot::MethodInfo("match_position_completed"));
    ADD_SIGNAL(godot::MethodInfo("match_rotation_started"));
    ADD_SIGNAL(godot::MethodInfo("match_rotation_completed"));
    ADD_SIGNAL(godot::MethodInfo("match_scale_started"));
    ADD_SIGNAL(godot::MethodInfo("match_scale_completed"));

    // Enums
    BIND_ENUM_CONSTANT(AgentExclusivity::NONE);
    BIND_ENUM_CONSTANT(AgentExclusivity::EXCLUSIVE);
    BIND_ENUM_CONSTANT(AgentExclusivity::GROUP_INCLUSIVE);
    BIND_ENUM_CONSTANT(AgentExclusivity::GROUP_EXCLUSIVE);

    BIND_ENUM_CONSTANT(AgentMatching::MATCH_NONE);
    BIND_ENUM_CONSTANT(AgentMatching::PLAYER_TO_AGENT);
    BIND_ENUM_CONSTANT(AgentMatching::AGENT_TO_PLAYER);
    BIND_ENUM_CONSTANT(AgentMatching::AVERAGE);

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_player", "new_player"), &GameAgent::set_player);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player"), &GameAgent::get_player);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer"), "set_player", "get_player");

    // Additional typical properties bindings (match_position, actions, game_pieces, etc...) omitted for brevity. Map structurally per standard export logic.

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("start_position_match", "player_to_match"), &GameAgent::start_position_match, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("_position_match", "percent"), &GameAgent::_position_match);
    godot::ClassDB::bind_method(godot::D_METHOD("find_best_action", "instigating_action"), &GameAgent::find_best_action);
    godot::ClassDB::bind_method(godot::D_METHOD("handle_agent_request", "game_piece"), &GameAgent::handle_agent_request);
    // Bind all callable Action and GamePiece methods...
}

GameAgent::GameAgent() {}

GameAgent::~GameAgent() {}

// Setters / Getters
void GameAgent::set_player(GamePlayer* p_player) {
    if (p_player == _player) return;
    
    if (_player) {
        release_and_control(p_player);
    } else {
        start_control(p_player);
    }
}
GamePlayer* GameAgent::get_player() const { return _player; }

// Other Setters/Getters elided for pure mechanics representation...

// Player Functions
void GameAgent::find_and_possess_agent(godot::Variant on) {
    // Implement utilizing GamePlayer API
}

void GameAgent::find_and_swap_agent(godot::Variant on) {
    // Implement utilizing GamePlayer API
}

void GameAgent::start_control(GamePlayer* new_player) {
    if (_player == new_player) return;

    _check_position_match();
    _check_rotation_match();
    _check_scale_match();

    // Assumption: Checking if signal is connected
    // if (has_connections("player_control_started")) {
    //     _started_to_control = new_player;
    //     emit_signal("player_control_started", new_player);
    //     return;
    // }

    control(new_player);
}

void GameAgent::continue_control() {
    control(_started_to_control);
}

void GameAgent::control(GamePlayer* new_player) {
    if (_player == new_player) return;
    _player = new_player;

    // if (player_node_assignments) player_node_assignments->retarget(this, _player);
    // if (player_signal_assignments) player_signal_assignments->connect_signals(this, _player);

    emit_signal("player_controlled", _player);
}

void GameAgent::release_and_control(GamePlayer* to_control) {
    _waiting_to_control = to_control;
    start_release();
}

void GameAgent::start_release() {
    // if (has_connections("player_control_release_started")) {
    //     emit_signal("player_control_release_started");
    //     return;
    // }
    control_end();
}

void GameAgent::control_end() {
    GamePlayer* rel = _player;
    
    // Disconnect retargeters/signals...
    
    _player = nullptr;
    emit_signal("player_released", rel);
    
    // rel->release_avatar(this);
    
    if (_waiting_to_control) {
        start_control(_waiting_to_control);
        _waiting_to_control = nullptr;
    }
}

// Matching Functions
void GameAgent::start_position_match(GamePlayer* player_to_match) {
    _position_match_complete = false;
    // if signal connections exist... emit else _position_match(1.0f);
}
void GameAgent::_position_match(float percent) { /* Interpolation logic */ }
void GameAgent::complete_position_match() {
    _position_match_complete = true;
    emit_signal("match_position_completed");
    _check_all_matches();
}

void GameAgent::start_rotation_match(GamePlayer* player_to_match) {
    _rotation_match_complete = false;
}
void GameAgent::_rotation_match(float percent) {}
void GameAgent::complete_rotation_match() {
    _rotation_match_complete = true;
    emit_signal("match_rotation_completed");
    _check_all_matches();
}

void GameAgent::start_scale_match(GamePlayer* player_to_match) {
    _scale_match_complete = false;
}
void GameAgent::_scale_match(float percent) {}
void GameAgent::complete_scale_match() {
    _scale_match_complete = true;
    emit_signal("match_scale_completed");
    _check_all_matches();
}

void GameAgent::_check_position_match() {}
void GameAgent::_check_rotation_match() {}
void GameAgent::_check_scale_match() {}
void GameAgent::_check_all_matches() {
    bool p = (match_position == AgentMatching::MATCH_NONE || _position_match_complete);
    bool r = (match_rotation == AgentMatching::MATCH_NONE || _rotation_match_complete);
    bool s = (match_scale == AgentMatching::MATCH_NONE || _scale_match_complete);
    
    if (p && r && s) {
        continue_control();
    }
}

// Action Functions
godot::TypedArray<godot::String> GameAgent::gather_action_titles() const {
    godot::TypedArray<godot::String> names;
    // For loop filling titles
    return names;
}

int GameAgent::add_action(GameAgentAction* new_action) {
    int id = get_action_id(new_action);
    if (id < 0) {
        id = actions.size();
        actions.append(new_action);
        emit_signal("action_added", new_action);
    }
    return id;
}

int GameAgent::get_action_id_from_title(const godot::String& title) const {
    for (int i = 0; i < actions.size(); ++i) {
        // if cast->get_name() == title return i;
    }
    return -1;
}

int GameAgent::get_action_id(GameAgentAction* action) const {
    return actions.find(action);
}

bool GameAgent::has_action(GameAgentAction* action) const {
    return get_action_id(action) >= 0;
}

bool GameAgent::remove_action(GameAgentAction* action) {
    int id = get_action_id(action);
    if (id >= 0) return remove_action_at(id);
    return false;
}

bool GameAgent::remove_action_at(int action_ID) {
    if (action_ID < 0 || action_ID >= actions.size()) return false;
    
    GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_ID]);
    actions.remove_at(action_ID); // DOD NOTE: Use std::vector fast erase here instead.
    
    if (action) emit_signal("action_removed", action);
    return true;
}

godot::Array GameAgent::find_best_action(GameAgentAction* instigating_action) const {
    godot::Array result;
    int action_id = -1;
    bool competing = false;
    
    // Logic evaluating scores across all actions based on groups/properties.
    
    result.push_back(action_id);
    result.push_back(competing);
    return result;
}

void GameAgent::start_action(int action_id, int target_id) {
    if (!get_enabled() || action_id < 0 || action_id >= actions.size()) return;
    
    if (target_id >= 0 && target_id < game_pieces.size()) {
        GamePiece* target = godot::Object::cast_to<GamePiece>(game_pieces[target_id]);
        targeted_action(action_id, target);
        return;
    }
    // Call start logic
}

void GameAgent::targeted_action(int action_id, GamePiece* target) {
    if (!get_enabled() || action_id < 0 || action_id >= actions.size()) return;
    // Call targeted logic
}

void GameAgent::end_action(int action_id) {}
void GameAgent::interrupt_action(int action_id) {}
void GameAgent::fail_action(int action_id, int margin_of_failure) {}
void GameAgent::complete_action(int action_id, int margin_of_victory) {}
int GameAgent::get_action_value(int action_id) const { return 0; }
void GameAgent::apply_action_value_property(const godot::NodePath& to, int action_id, const godot::String& property_path) {}
void GameAgent::apply_action_value_method(const godot::NodePath& to, int action_id, const godot::String& method_name) {}

GameInteraction* GameAgent::interact(int action_id) { return nullptr; }
void GameAgent::join_interaction(GameInteraction* interaction) {}
void GameAgent::leave_interaction(GameInteraction* interaction) {}
void GameAgent::stop_interacting(int action_id) {}

void GameAgent::action_consequences(int score, const godot::Dictionary& consequences) {
    GameEntity::action_consequences(score, consequences);
    
    // DOD NOTE: Dictionary lookups per-frame or during dense loop evaluations are highly detrimental. 
    // This logic dynamically parses string keys to mutate entity state. In C++, utilize a robust Command 
    // pattern with strongly typed structs to guarantee type safety and contiguous execution.
}

// Game Piece Functions
godot::TypedArray<godot::String> GameAgent::gather_game_piece_titles() const { return godot::TypedArray<godot::String>(); }
int GameAgent::add_game_piece(GamePiece* new_game_piece) { return -1; }
int GameAgent::get_game_piece_id(GamePiece* game_piece) const { return -1; }
bool GameAgent::has_game_piece(GamePiece* game_piece) const { return false; }
bool GameAgent::remove_game_piece(GamePiece* game_piece) { return false; }
bool GameAgent::remove_game_piece_at(int game_piece_ID) { return false; }
void GameAgent::handle_agent_request(GamePiece* game_piece) {}

// GameEntity Overrides
void GameAgent::enter_game() { GameEntity::enter_game(); }
void GameAgent::game_start() { GameEntity::game_start(); }
void GameAgent::game_pause() { GameEntity::game_pause(); }
void GameAgent::game_continue() { GameEntity::game_continue(); }
void GameAgent::game_end() { GameEntity::game_end(); }
void GameAgent::exit_game() { GameEntity::exit_game(); }

void GameAgent::game_process(double delta) {
    // DOD NOTE: Iterating through an array of Godot Objects to invoke a virtual function 
    // inherently fragments cache. C++ refactoring should decouple 'Actions' and 'GamePieces' 
    // from Node hierarchies, updating pure arrays of floats/ints linearly for maximum performance.
    
    // if (game_processed) return;
    // double agent_delta = delta * get_time_scale();
    // ...
    // game_processed = true;
}

void GameAgent::game_process_clear() {}

godot::Dictionary GameAgent::save_data() const {
    return GameEntity::save_data();
}

void GameAgent::load_data(const godot::Dictionary& data) {}

} // namespace ideam::godot_ext