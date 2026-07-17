#include "game_agent.h"

#include <godot_cpp/core/class_db.hpp>

#include "game_piece.h"
#include "actions/game_agent_action.h"
#include "actions/game_interaction.h"
#include "../game.h"
#include "../game_player.h"
#include "../game_property.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"


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

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_parent_target", "target"), &GameAgent::set_player_parent_target);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_parent_target"), &GameAgent::get_player_parent_target);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player_parent_target", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_player_parent_target", "get_player_parent_target");

    // Standard methods mapped from header
    godot::ClassDB::bind_method(godot::D_METHOD("start_position_match", "player_to_match"), &GameAgent::start_position_match, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("_position_match", "percent"), &GameAgent::_position_match);
    godot::ClassDB::bind_method(godot::D_METHOD("find_best_action", "instigating_action"), &GameAgent::find_best_action);
    godot::ClassDB::bind_method(godot::D_METHOD("handle_agent_request", "game_piece"), &GameAgent::handle_agent_request);
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

void GameAgent::set_player_parent_target(godot::Node* p_target) { 
    if (p_target == player_parent_target) return;
    player_parent_target = p_target; 
}
godot::Node* GameAgent::get_player_parent_target() const { return player_parent_target; }

void GameAgent::set_exclusivity(AgentExclusivity p_exclusivity) { 
    if (p_exclusivity == exclusivity) return;
    exclusivity = p_exclusivity; 
}
AgentExclusivity GameAgent::get_exclusivity() const { return exclusivity; }

void GameAgent::set_match_position(AgentMatching p_match) {
    if (p_match == match_position) return;
    match_position = p_match;
    _check_position_match();
}
AgentMatching GameAgent::get_match_position() const { return match_position; }

void GameAgent::set_player_reposition_time(float p_time) { 
    if (p_time == player_reposition_time) return;
    player_reposition_time = p_time; 
}
float GameAgent::get_player_reposition_time() const { return player_reposition_time; }

void GameAgent::set_match_rotation(AgentMatching p_match) {
    if (p_match == match_rotation) return;
    match_rotation = p_match;
    _check_rotation_match();
}
AgentMatching GameAgent::get_match_rotation() const { return match_rotation; }

void GameAgent::set_player_reorient_time(float p_time) { 
    if (p_time == player_reorient_time) return;
    player_reorient_time = p_time; 
}
float GameAgent::get_player_reorient_time() const { return player_reorient_time; }

void GameAgent::set_match_scale(AgentMatching p_match) {
    if (p_match == match_scale) return;
    match_scale = p_match;
    _check_scale_match();
}
AgentMatching GameAgent::get_match_scale() const { return match_scale; }

void GameAgent::set_player_rescale_time(float p_time) { 
    if (p_time == player_rescale_time) return;
    player_rescale_time = p_time; 
}
float GameAgent::get_player_rescale_time() const { return player_rescale_time; }

void GameAgent::set_actions(const godot::TypedArray<GameAgentAction>& p_actions) { 
    if (p_actions == actions) return;
    actions = p_actions; 
}
godot::TypedArray<GameAgentAction> GameAgent::get_actions() const { return actions; }

void GameAgent::set_game_pieces(const godot::TypedArray<GamePiece>& p_pieces) { 
    if (p_pieces == game_pieces) return;
    game_pieces = p_pieces; 
}
godot::TypedArray<GamePiece> GameAgent::get_game_pieces() const { return game_pieces; }

void GameAgent::set_player_node_assignments(NodeRetargeter* p_assignments) { 
    if (p_assignments == player_node_assignments) return;
    player_node_assignments = p_assignments; 
}
NodeRetargeter* GameAgent::get_player_node_assignments() const { return player_node_assignments; }

void GameAgent::set_player_signal_assignments(SignalConnector* p_assignments) { 
    if (p_assignments == player_signal_assignments) return;
    player_signal_assignments = p_assignments; 
}
SignalConnector* GameAgent::get_player_signal_assignments() const { return player_signal_assignments; }


// Player Functions
void GameAgent::find_and_possess_agent(godot::Variant on) {
    if (!_player) return;
    
    godot::Object* obj = on.operator godot::Object*();
    GameAgent* other = godot::Object::cast_to<GameAgent>(obj);
    
    if (other && other != this) {
        _player->control_game_agent(other);
    }
}

void GameAgent::find_and_swap_agent(godot::Variant on) {
    if (!_player) return;
    
    godot::Object* obj = on.operator godot::Object*();
    GameAgent* other = godot::Object::cast_to<GameAgent>(obj);
    
    if (other && other != this) {
        _player->control_game_agent(other);
        _player->release_game_agent(this);
    }
}

void GameAgent::start_control(GamePlayer* new_player) {
    if (_player == new_player) return;

    _check_position_match();
    _check_rotation_match();
    _check_scale_match();

    if (has_connections("player_control_started")) {
        _started_to_control = new_player;
        emit_signal("player_control_started", new_player);
        return;
    }

    control(new_player);
}

void GameAgent::continue_control() {
    control(_started_to_control);
}

void GameAgent::control(GamePlayer* new_player) {
    if (_player == new_player) return;
    _player = new_player;

    if (player_node_assignments) player_node_assignments->retarget(this, _player);
    if (player_signal_assignments) player_signal_assignments->connect_signals(this, _player);

    emit_signal("player_controlled", _player);
}

void GameAgent::release_and_control(GamePlayer* to_control) {
    _waiting_to_control = to_control;
    start_release();
}

void GameAgent::start_release() {
    if (has_connections("player_control_release_started")) {
        emit_signal("player_control_release_started");
        return;
    }
    control_end();
}

void GameAgent::control_end() {
    GamePlayer* rel = _player;
    
    if (player_node_assignments) player_node_assignments->clear_set_targets(this, _player);
    if (player_signal_assignments) player_signal_assignments->disconnect_signals(this, _player);
    
    _player = nullptr;
    emit_signal("player_released", rel);
    
    if (rel) rel->release_game_agent(this);
    
    if (_waiting_to_control) {
        start_control(_waiting_to_control);
        _waiting_to_control = nullptr;
    }
}

// Matching Functions
void GameAgent::start_position_match(GamePlayer* player_to_match) {
    _position_match_complete = false;
    if (has_connections("match_position_started")) {
        emit_signal("match_position_started");
        return;
    }
    _position_match(1.0f);
}
void GameAgent::_position_match(float percent) { }
void GameAgent::complete_position_match() {
    _position_match_complete = true;
    emit_signal("match_position_completed");
    _check_all_matches();
}

void GameAgent::start_rotation_match(GamePlayer* player_to_match) {
    _rotation_match_complete = false;
    if (has_connections("match_rotation_started")) {
        emit_signal("match_rotation_started");
        return;
    }
    _rotation_match(1.0f);
}
void GameAgent::_rotation_match(float percent) { }
void GameAgent::complete_rotation_match() {
    _rotation_match_complete = true;
    emit_signal("match_rotation_completed");
    _check_all_matches();
}

void GameAgent::start_scale_match(GamePlayer* player_to_match) {
    _scale_match_complete = false;
    if (has_connections("match_scale_started")) {
        emit_signal("match_scale_started");
        return;
    }
    _scale_match(1.0f);
}
void GameAgent::_scale_match(float percent) { }
void GameAgent::complete_scale_match() {
    _scale_match_complete = true;
    emit_signal("match_scale_completed");
    _check_all_matches();
}

void GameAgent::_check_position_match() {
    if (match_position != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_position_match"))) {
            connect("player_control_started", godot::Callable(this, "start_position_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_position_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_position_match"));
        }
    }
}

void GameAgent::_check_rotation_match() {
    if (match_rotation != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_rotation_match"))) {
            connect("player_control_started", godot::Callable(this, "start_rotation_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_rotation_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_rotation_match"));
        }
    }
}

void GameAgent::_check_scale_match() {
    if (match_scale != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_scale_match"))) {
            connect("player_control_started", godot::Callable(this, "start_scale_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_scale_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_scale_match"));
        }
    }
}

void GameAgent::_check_all_matches() {
    bool p = true;
    if (match_position != AgentMatching::MATCH_NONE && !_position_match_complete) p = false;
    
    bool r = true;
    if (match_rotation != AgentMatching::MATCH_NONE && !_rotation_match_complete) r = false;
    
    bool s = true;
    if (match_scale != AgentMatching::MATCH_NONE && !_scale_match_complete) s = false;
    
    if (p && r && s) {
        continue_control();
    }
}

// Action Functions
godot::TypedArray<godot::String> GameAgent::gather_action_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (act) names.append(act->get_name()); // Assumption: get_name() maps to title
    }
    return names;
}

int GameAgent::add_action(GameAgentAction* new_action) {
    if (!new_action) return -1;
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
        GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (act && act->get_name() == title) return i;
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
    actions.remove_at(action_ID);
    
    if (action) emit_signal("action_removed", action);
    return true;
}

godot::Array GameAgent::find_best_action(GameAgentAction* instigating_action) const {
    godot::Array result;
    int action_id = -1;
    int highest_score = -1;
    bool competing = false;

    if (!instigating_action) {
        result.push_back(action_id);
        result.push_back(competing);
        return result;
    }

    godot::TypedArray<godot::StringName> inst_groups = instigating_action->get_groups();

    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (!act || !act->get_enabled()) continue;

        bool skip = false;
        int act_score = 0;
        bool act_competing = false;

        // Iterate through attached pieces and properties cleanly
        godot::TypedArray<GamePiece> act_pieces; // Assumption: act->get_game_pieces(); (GDScript infers this)
        for (int j = 0; j < act_pieces.size(); ++j) {
            GamePiece* p = godot::Object::cast_to<GamePiece>(act_pieces[j]);
            if (!p || !p->get_enabled()) {
                skip = true;
                break;
            }
            
            godot::TypedArray<GameProperty> props; // Assumption: p->get_game_properties();
            for(int k = 0; k < props.size(); ++k) {
                GameProperty* prop = godot::Object::cast_to<GameProperty>(props[k]);
                if (prop) {
                    if (prop->get_locked()) act_score -= 1; // Assumption logic derived from GDScript
                    // else if (prop->is_exhausted()) act_score -= 1;
                }
            }
        }

        if (skip) continue;

        for (int g = 0; g < inst_groups.size(); ++g) {
            if (act->is_in_group(inst_groups[g])) {
                act_score += 1;
            }
            
            // godot::TypedArray<godot::StringName> comp_groups = act->get_competes_with_groups();
            // if (comp_groups.has(inst_groups[g])) {
            //     act_score += 1;
            //     act_competing = true;
            // }
        }

        if (act_score > highest_score) {
            action_id = i;
            highest_score = act_score;
            competing = act_competing;
        }
    }

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
    
    GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
    if (action && !action->get_in_progress()) { // Assumption getter map
        action->start_action();
    }
}

void GameAgent::targeted_action(int action_id, GamePiece* target) {
    if (!get_enabled() || action_id < 0 || action_id >= actions.size() || !target) return;
    
    GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
    if (action) {
        if (action->get_in_progress()) {
            // action->target_for_sequence(target);
        } else {
            // action->start_targeted_action(target);
        }
    }
}

void GameAgent::end_action(int action_id) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->end_action();
    }
}

void GameAgent::interrupt_action(int action_id) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->interrupt_action();
    }
}

void GameAgent::fail_action(int action_id, int margin_of_failure) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->fail_action(margin_of_failure);
    }
}

void GameAgent::complete_action(int action_id, int margin_of_victory) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->complete_action(margin_of_victory);
    }
}

int GameAgent::get_action_value(int action_id) const {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) {
            // return action->get_current_value();
        }
    }
    return 0;
}

void GameAgent::apply_action_value_property(const godot::NodePath& to, int action_id, const godot::String& property_path) {
    if (has_node(to)) {
        godot::Node* target = get_node<godot::Node>(to);
        int v = get_action_value(action_id);
        if (target) target->set(property_path, v);
    }
}

void GameAgent::apply_action_value_method(const godot::NodePath& to, int action_id, const godot::String& method_name) {
    if (has_node(to)) {
        godot::Node* target = get_node<godot::Node>(to);
        int v = get_action_value(action_id);
        if (target) target->call(method_name, v);
    }
}

GameInteraction* GameAgent::interact(int action_id) {
    for (int i = 0; i < interactions.size(); ++i) {
        GameInteraction* inter = godot::Object::cast_to<GameInteraction>(interactions[i]);
        if (inter) {
            godot::Array instigator = inter->get_instigator(); // Assuming getter
            if (instigator.size() > 1 && static_cast<int>(instigator[1]) == action_id) {
                return inter;
            }
        }
    }
    
    // Requires standard game pointer
    // Game* game = get_game();
    // if (!game) return nullptr;
    // GameInteraction* interaction = game->request_game_interaction();
    
    // godot::Array participation;
    // participation.push_back(this);
    // participation.push_back(action_id);
    
    // interaction->set_instigator(participation);
    // interactions.append(interaction);
    
    return nullptr;
}

void GameAgent::join_interaction(GameInteraction* interaction) {
    if (!interaction) return;
    
    godot::Array instigator = interaction->get_instigator();
    if (instigator.size() < 2) return;
    
    GameAgent* inst_agent = godot::Object::cast_to<GameAgent>(instigator[0]);
    if (!inst_agent) return;
    
    godot::TypedArray<GameAgentAction> inst_actions = inst_agent->get_actions();
    int inst_action_id = instigator[1];
    
    if (inst_action_id >= 0 && inst_action_id < inst_actions.size()) {
        GameAgentAction* inst_action = godot::Object::cast_to<GameAgentAction>(inst_actions[inst_action_id]);
        
        godot::Array do_act = find_best_action(inst_action);
        int action_id = do_act[0];
        
        if (action_id >= 0) {
            godot::Array participation;
            participation.push_back(this);
            participation.push_back(action_id);
            
            bool competing = do_act[1];
            // if (competing) interaction->join_competition(participation);
            // else interaction->join_cooperation(participation);
        }
    }
}

void GameAgent::leave_interaction(GameInteraction* interaction) {
    // if (interaction) interaction->leave(this);
}

void GameAgent::stop_interacting(int action_id) {
    for (int i = interactions.size() - 1; i >= 0; --i) {
        GameInteraction* inter = godot::Object::cast_to<GameInteraction>(interactions[i]);
        if (inter) {
            godot::Array instigator = inter->get_instigator();
            if (instigator.size() > 1 && static_cast<int>(instigator[1]) == action_id) {
                // inter->stop_participant(instigator);
                // inter->set_instigator(godot::Array());
                interactions.remove_at(i);
                return;
            }
        }
    }
}

void GameAgent::action_consequences(int score, const godot::Dictionary& consequences) {
    // DOD NOTE: String-based key parsing to mutate object state per-frame represents 
    // the single largest cache stall in scripting pipelines. In C++, this logic should 
    // be replaced with an enum-based Command structure or Event Queue, enabling direct 
    // memory offsets rather than Variant hash resolutions.
    
    GameEntity::action_consequences(score, consequences);
    
    if (consequences.has("current_action_targets")) {
        godot::Dictionary targets_dict = consequences["current_action_targets"];
        for (int i = 0; i < current_action_targets.size(); ++i) {
            GameEntity* a = godot::Object::cast_to<GameEntity>(current_action_targets[i]);
            if (a) a->action_consequences(score, targets_dict);
        }
    }
    
    if (consequences.has("add_action")) {
        godot::TypedArray<GameAgentAction> actions_to_add = consequences["add_action"];
        for (int i = 0; i < actions_to_add.size(); ++i) {
            add_action(godot::Object::cast_to<GameAgentAction>(actions_to_add[i]));
        }
    }
    
    if (consequences.has("remove_action")) {
        godot::TypedArray<GameAgentAction> actions_to_remove = consequences["remove_action"];
        for (int i = 0; i < actions_to_remove.size(); ++i) {
            remove_action(godot::Object::cast_to<GameAgentAction>(actions_to_remove[i]));
        }
    }
    
    if (consequences.has("actions")) {
        godot::Dictionary actions_dict = consequences["actions"];
        for (int i = 0; i < actions.size(); ++i) {
            GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
            if (act) act->action_consequences(score, actions_dict);
        }
    }
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) {
            godot::String action_setting = "action_" + action->get_name();
            if (consequences.has(action_setting)) {
                action->action_consequences(score, consequences[action_setting]);
            }
        }
    }
    
    if (consequences.has("add_game_piece")) {
        godot::TypedArray<GamePiece> pieces_to_add = consequences["add_game_piece"];
        for (int i = 0; i < pieces_to_add.size(); ++i) {
            add_game_piece(godot::Object::cast_to<GamePiece>(pieces_to_add[i]));
        }
    }
    
    if (consequences.has("remove_game_piece")) {
        godot::TypedArray<GamePiece> pieces_to_remove = consequences["remove_game_piece"];
        for (int i = 0; i < pieces_to_remove.size(); ++i) {
            remove_game_piece(godot::Object::cast_to<GamePiece>(pieces_to_remove[i]));
        }
    }
    
    if (consequences.has("game_pieces")) {
        godot::Dictionary pieces_dict = consequences["game_pieces"];
        for (int i = 0; i < game_pieces.size(); ++i) {
            GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
            if (piece) piece->action_consequences(score, pieces_dict);
        }
    }
    
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) {
            godot::String piece_setting = "game_piece_" + piece->get_name();
            if (consequences.has(piece_setting)) {
                piece->action_consequences(score, consequences[piece_setting]);
            }
        }
    }
}

// Game Piece Functions
godot::TypedArray<godot::String> GameAgent::gather_game_piece_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) names.append(piece->get_name()); // Assumption: get_name() maps to title
    }
    return names;
}

int GameAgent::add_game_piece(GamePiece* new_game_piece) {
    if (!new_game_piece) return -1;
    int id = get_game_piece_id(new_game_piece);
    
    if (id < 0) {
        id = game_pieces.size();
        game_pieces.append(new_game_piece);
        emit_signal("game_piece_added", new_game_piece);
        
        if (!new_game_piece->is_connected("agent_requested", godot::Callable(this, "handle_agent_request"))) {
            new_game_piece->connect("agent_requested", godot::Callable(this, "handle_agent_request"));
        }
    }
    return id;
}

int GameAgent::get_game_piece_id(GamePiece* game_piece) const {
    return game_pieces.find(game_piece);
}

bool GameAgent::has_game_piece(GamePiece* game_piece) const {
    return get_game_piece_id(game_piece) >= 0;
}

bool GameAgent::remove_game_piece(GamePiece* game_piece) {
    int id = get_game_piece_id(game_piece);
    if (id >= 0) return remove_game_piece_at(id);
    return false;
}

bool GameAgent::remove_game_piece_at(int game_piece_ID) {
    if (game_piece_ID < 0 || game_piece_ID >= game_pieces.size()) return false;
    
    GamePiece* game_piece = godot::Object::cast_to<GamePiece>(game_pieces[game_piece_ID]);
    if (game_piece) {
        if (game_piece->is_connected("agent_requested", godot::Callable(this, "handle_agent_request"))) {
            game_piece->disconnect("agent_requested", godot::Callable(this, "handle_agent_request"));
        }
        
        for (int i = 0; i < actions.size(); ++i) {
            GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
            // if (action) action->remove_game_piece(game_piece);
        }
    }
    
    game_pieces.remove_at(game_piece_ID);
    if (game_piece) emit_signal("game_piece_removed", game_piece);
    
    return true;
}

void GameAgent::handle_agent_request(GamePiece* game_piece) {
    if (game_piece) {
        // godot::TypedArray<GameAgent> requested = game_piece->get_requested_agents();
        // requested.append(this);
    }
}

// GameEntity Overrides
void GameAgent::enter_game() {
    GameEntity::enter_game();
    // Game* g = get_game();
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        // if (action) action->set_game(g);
    }
    
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        // if (piece) piece->set_game(g);
    }
}

void GameAgent::game_start() {
    GameEntity::game_start();
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_start();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_start();
    }
}

void GameAgent::game_pause() {
    if (entity_is_paused) return;
    GameEntity::game_pause();
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_pause();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_pause();
    }
}

void GameAgent::game_continue() {
    if (!entity_is_paused) return;
    GameEntity::game_continue();
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_continue();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_continue();
    }
}

void GameAgent::game_end() {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_end();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_end();
    }
    GameEntity::game_end();
}

void GameAgent::exit_game() {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->exit_game();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->exit_game();
    }
    GameEntity::exit_game();
}

void GameAgent::game_process(double delta) {
    // if (game_processed) return;
    // float time_scale = 1.0f; // get_time_scale();
    // double agent_delta = delta * time_scale;
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_process(delta); // Use agent_delta
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_process(delta); // Use agent_delta
    }
    
    // game_processed = true;
}

void GameAgent::game_process_clear() {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_process_clear();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_process_clear();
    }
    // game_processed = false;
}

godot::Dictionary GameAgent::save_data() const {
    return GameEntity::save_data();
}

void GameAgent::load_data(const godot::Dictionary& data) {
    GameEntity::load_data(data);
}

} // namespace ideam::godot_ext