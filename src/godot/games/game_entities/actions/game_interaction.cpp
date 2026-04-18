#include "game_interaction.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

// Forward decl headers to be supplied by project
#include "../game_agent.h"
#include "../game_piece.h"
#include "game_agent_action.h"
#include "game_piece_action.h"

namespace ideam::godot_ext {

void GameInteraction::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("begun"));
    ADD_SIGNAL(godot::MethodInfo("concluded"));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_instigator", "instigator"), &GameInteraction::set_instigator);
    godot::ClassDB::bind_method(godot::D_METHOD("get_instigator"), &GameInteraction::get_instigator);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "instigator"), "set_instigator", "get_instigator");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cooperation", "cooperation"), &GameInteraction::set_cooperation);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cooperation"), &GameInteraction::get_cooperation);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "cooperation"), "set_cooperation", "get_cooperation");

    godot::ClassDB::bind_method(godot::D_METHOD("set_competition", "competition"), &GameInteraction::set_competition);
    godot::ClassDB::bind_method(godot::D_METHOD("get_competition"), &GameInteraction::get_competition);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "competition"), "set_competition", "get_competition");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("begin_interacting", "inst"), &GameInteraction::begin_interacting);
    godot::ClassDB::bind_method(godot::D_METHOD("join_cooperation", "c"), &GameInteraction::join_cooperation);
    godot::ClassDB::bind_method(godot::D_METHOD("cooperation_id", "entity"), &GameInteraction::cooperation_id);
    godot::ClassDB::bind_method(godot::D_METHOD("leave_cooperation", "cid"), &GameInteraction::leave_cooperation);
    godot::ClassDB::bind_method(godot::D_METHOD("join_competition", "c"), &GameInteraction::join_competition);
    godot::ClassDB::bind_method(godot::D_METHOD("competition_id", "entity"), &GameInteraction::competition_id);
    godot::ClassDB::bind_method(godot::D_METHOD("leave_competition", "cid"), &GameInteraction::leave_competition);
    godot::ClassDB::bind_method(godot::D_METHOD("leave", "entity"), &GameInteraction::leave);
    godot::ClassDB::bind_method(godot::D_METHOD("prep_participant", "participant"), &GameInteraction::prep_participant);
    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_interaction", "stepped_agent", "sequence_id"), &GameInteraction::evaluate_interaction);
    godot::ClassDB::bind_method(godot::D_METHOD("score_participant", "participant"), &GameInteraction::score_participant);
    godot::ClassDB::bind_method(godot::D_METHOD("resolve_participant", "participant", "score_diff"), &GameInteraction::resolve_participant);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_consequences", "participant", "score", "consequences"), &GameInteraction::apply_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("stop_interacting"), &GameInteraction::stop_interacting);
    godot::ClassDB::bind_method(godot::D_METHOD("stop_participant", "participant"), &GameInteraction::stop_participant);
}

GameInteraction::GameInteraction() {}

GameInteraction::~GameInteraction() {}

// Setters / Getters
void GameInteraction::set_instigator(const godot::Array& p_instigator) { instigator = p_instigator; }
godot::Array GameInteraction::get_instigator() const { return instigator; }

void GameInteraction::set_cooperation(const godot::Array& p_cooperation) { cooperation = p_cooperation; }
godot::Array GameInteraction::get_cooperation() const { return cooperation; }

void GameInteraction::set_competition(const godot::Array& p_competition) { competition = p_competition; }
godot::Array GameInteraction::get_competition() const { return competition; }

// Class Functions
void GameInteraction::begin_interacting(const godot::Array& inst) {
    instigator = inst;
    
    if (instigator.size() <= 1) {
        instigator.clear();
        return;
    }
    
    interaction_active = true;
    sequence_connections.clear();
    
    prep_participant(instigator);
}

void GameInteraction::join_cooperation(const godot::Array& c) {
    cooperation.append(c);
    prep_participant(c);
}

int GameInteraction::cooperation_id(godot::Object* entity) const {
    for (int i = 0; i < cooperation.size(); ++i) {
        godot::Array c = cooperation[i];
        if (c.size() > 0 && godot::Object::cast_to<godot::Object>(c[0]) == entity) {
            return i;
        }
    }
    return -1;
}

void GameInteraction::leave_cooperation(int cid) {
    if (cid < 0 || cid >= cooperation.size()) return;
    
    stop_participant(cooperation[cid]);
    cooperation.remove_at(cid);
}

void GameInteraction::join_competition(const godot::Array& c) {
    competition.append(c);
    prep_participant(c);
}

int GameInteraction::competition_id(godot::Object* entity) const {
    for (int i = 0; i < competition.size(); ++i) {
        godot::Array c = competition[i];
        if (c.size() > 0 && godot::Object::cast_to<godot::Object>(c[0]) == entity) {
            return i;
        }
    }
    return -1;
}

void GameInteraction::leave_competition(int cid) {
    if (cid < 0 || cid >= competition.size()) return;
    
    stop_participant(competition[cid]);
    competition.remove_at(cid);
}

void GameInteraction::leave(godot::Object* entity) {
    for (int c = 0; c < competition.size(); ++c) {
        godot::Array comp_arr = competition[c];
        if (comp_arr.size() > 0 && godot::Object::cast_to<godot::Object>(comp_arr[0]) == entity) {
            leave_competition(c);
            return;
        }
    }
            
    for (int c = 0; c < cooperation.size(); ++c) {
        godot::Array coop_arr = cooperation[c];
        if (coop_arr.size() > 0 && godot::Object::cast_to<godot::Object>(coop_arr[0]) == entity) {
            leave_cooperation(c);
            return;
        }
    }
}

void GameInteraction::prep_participant(const godot::Array& participant) {
    if (participant.size() < 2) return;

    if (GameAgent* agent = godot::Object::cast_to<GameAgent>(participant[0])) {
        // Implement Action connection bindings
        // Assuming GameAgent::get_actions() -> Array
        // int action_id = participant[1];
        // GameAgentAction* agent_action = cast(agent.actions[action_id]);
        
        // C++ equivalent to GDScript lambda bindings:
        // godot::Callable agent_sequence = godot::Callable(this, "evaluate_interaction").bind(agent);
        // agent_action->connect("sequence_stepped", agent_sequence);
        // sequence_connections[agent_action] = agent_sequence;
    } else if (godot::Object::cast_to<GamePiece>(participant[0])) {
        // Implement GamePiece prep
    }
}

void GameInteraction::evaluate_interaction(GameAgent* stepped_agent, int sequence_id) {
    instigator_value = 0;
    competition_value = 0;
    
    godot::Array participant;
    
    if (instigator.size() > 1) {
        participant = instigator;
        instigator_value += score_participant(participant);
    }
    
    for (int i = 0; i < cooperation.size(); ++i) {
        participant = cooperation[i];
        instigator_value += score_participant(participant);
    }
    
    for (int i = 0; i < competition.size(); ++i) {
        participant = competition[i];
        competition_value += score_participant(participant);
    }
    
    int score_diff = instigator_value - competition_value;
    
    if (instigator.size() > 1) {
        participant = instigator;
        resolve_participant(participant, score_diff);
        if (godot::Object::cast_to<GamePiece>(participant[0])) {
            instigator.clear();
        }
    }
    
    for (int p = cooperation.size() - 1; p >= 0; --p) {
        godot::Array coop_p = cooperation[p];
        resolve_participant(coop_p, score_diff);
        if (godot::Object::cast_to<GamePiece>(coop_p[0])) {
            cooperation.remove_at(p);
        }
    }
    
    for (int p = competition.size() - 1; p >= 0; --p) {
        godot::Array comp_p = competition[p];
        resolve_participant(comp_p, score_diff);
        if (godot::Object::cast_to<GamePiece>(comp_p[0])) {
            competition.remove_at(p);
        }
    }
}

int GameInteraction::score_participant(const godot::Array& participant) const {
    if (participant.size() < 2) return 0;

    if (GameAgent* agent = godot::Object::cast_to<GameAgent>(participant[0])) {
        int action_id = participant[1];
        // return agent->get_action_value(action_id);
        return 0; // Placeholder
    } else if (godot::Object::cast_to<GamePiece>(participant[0])) {
        // GamePiece* piece = ...
        // int action_id = participant[1];
        // return piece->get_action_value(action_id);
        return 0; // Placeholder
    }
    
    return 0;
}

void GameInteraction::resolve_participant(const godot::Array& participant, int score_diff) {
    if (participant.size() < 2) return;

    if (GameAgent* agent = godot::Object::cast_to<GameAgent>(participant[0])) {
        // Implement action sequence resolution mapping
        // int action_id = participant[1];
        // agent_action = agent.actions[action_id];
        // iterate step sequences -> apply_consequences
    } else if (godot::Object::cast_to<GamePiece>(participant[0])) {
        // int action_id = participant[1];
        // piece_action = piece.actions[action_id];
        // piece_action.action_success() or failure()
    }
}

void GameInteraction::apply_consequences(GameEntity* participant, int score, const godot::Dictionary& consequences) {
    godot::Array keys = consequences.keys();
    godot::Array vals = consequences.values();
    
    for (int i = 0; i < consequences.size(); ++i) {
        if (GameEntity* entity_key = godot::Object::cast_to<GameEntity>(keys[i])) {
            entity_key->action_consequences(score, vals[i]);
        } else {
            godot::String target = keys[i];
            
            if (target == "participant") {
                if (participant) participant->action_consequences(score, consequences);
            } else if (target == "all") {
                if (instigator.size() > 0) {
                    if (GameEntity* inst = godot::Object::cast_to<GameEntity>(instigator[0])) {
                        inst->action_consequences(score, vals[i]);
                    }
                }
                for (int p = 0; p < cooperation.size(); ++p) {
                    godot::Array cp = cooperation[p];
                    if (cp.size() > 0) {
                        if (GameEntity* cp_ent = godot::Object::cast_to<GameEntity>(cp[0])) cp_ent->action_consequences(score, vals[i]);
                    }
                }
                for (int p = 0; p < competition.size(); ++p) {
                    godot::Array cp = competition[p];
                    if (cp.size() > 0) {
                        if (GameEntity* cp_ent = godot::Object::cast_to<GameEntity>(cp[0])) cp_ent->action_consequences(score, vals[i]);
                    }
                }
            } else if (target == "instigator") {
                if (instigator.size() > 0) {
                    if (GameEntity* inst = godot::Object::cast_to<GameEntity>(instigator[0])) inst->action_consequences(score, vals[i]);
                }
            } else if (target == "cooperation") {
                for (int p = 0; p < cooperation.size(); ++p) {
                    godot::Array cp = cooperation[p];
                    if (cp.size() > 0) {
                        if (GameEntity* cp_ent = godot::Object::cast_to<GameEntity>(cp[0])) cp_ent->action_consequences(score, vals[i]);
                    }
                }
            } else if (target == "competition") {
                for (int p = 0; p < competition.size(); ++p) {
                    godot::Array cp = competition[p];
                    if (cp.size() > 0) {
                        if (GameEntity* cp_ent = godot::Object::cast_to<GameEntity>(cp[0])) cp_ent->action_consequences(score, vals[i]);
                    }
                }
            } else {
                if (instigator.size() > 0) {
                    GameEntity* inst = godot::Object::cast_to<GameEntity>(instigator[0]);
                    if (inst && target == inst->get_name()) { // Assuming name access
                        inst->action_consequences(score, vals[i]);
                    }
                }
                
                // Fallback loops for specific named entities in arrays
                for (int p = 0; p < cooperation.size(); ++p) {
                    godot::Array cp = cooperation[p];
                    if (cp.size() > 0) {
                        GameEntity* cp_ent = godot::Object::cast_to<GameEntity>(cp[0]);
                        if (cp_ent && cp_ent->get_name() == target) cp_ent->action_consequences(score, vals[i]);
                    }
                }
                for (int p = 0; p < competition.size(); ++p) {
                    godot::Array cp = competition[p];
                    if (cp.size() > 0) {
                        GameEntity* cp_ent = godot::Object::cast_to<GameEntity>(cp[0]);
                        if (cp_ent && cp_ent->get_name() == target) cp_ent->action_consequences(score, vals[i]);
                    }
                }
            }
        }
    }
}

void GameInteraction::stop_interacting() {
    if (instigator.size() > 1) stop_participant(instigator);
    for (int i = 0; i < cooperation.size(); ++i) stop_participant(cooperation[i]);
    for (int i = 0; i < competition.size(); ++i) stop_participant(competition[i]);
    
    interaction_active = false;
}

void GameInteraction::stop_participant(const godot::Array& participant) {
    if (participant.size() < 3) return; // Note: prep mapped index 2 based on your logic vs 1 previously.
    
    if (GameAgent* agent = godot::Object::cast_to<GameAgent>(participant[0])) {
        // Implement disconnection cleanup
        // agent_action->disconnect("sequence_stepped", agent_sequence);
    } else if (godot::Object::cast_to<GamePiece>(participant[0])) {
        // Implement GamePiece stop logic
    }
}

} // namespace ideam::godot_ext