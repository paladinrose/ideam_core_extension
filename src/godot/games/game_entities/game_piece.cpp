#include "game_piece.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "../game.h"
#include "../game_property.h"
#include "actions/game_piece_action.h"
#include "game_agent.h"
#include "actions/game_interaction.h"

namespace ideam::godot_ext {

void GamePiece::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("action_started", godot::PropertyInfo(godot::Variant::OBJECT, "action", godot::PROPERTY_HINT_NODE_TYPE, "GamePieceAction"), godot::PropertyInfo(godot::Variant::INT, "value")));
    ADD_SIGNAL(godot::MethodInfo("action_ended", godot::PropertyInfo(godot::Variant::OBJECT, "action", godot::PROPERTY_HINT_NODE_TYPE, "GamePieceAction")));
    ADD_SIGNAL(godot::MethodInfo("action_added", godot::PropertyInfo(godot::Variant::OBJECT, "new_action", godot::PROPERTY_HINT_NODE_TYPE, "GamePieceAction")));
    ADD_SIGNAL(godot::MethodInfo("action_removed", godot::PropertyInfo(godot::Variant::OBJECT, "removed", godot::PROPERTY_HINT_NODE_TYPE, "GamePieceAction")));

    ADD_SIGNAL(godot::MethodInfo("sub_piece_added", godot::PropertyInfo(godot::Variant::OBJECT, "sub_piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    ADD_SIGNAL(godot::MethodInfo("sub_piece_removed", godot::PropertyInfo(godot::Variant::OBJECT, "sub_piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    ADD_SIGNAL(godot::MethodInfo("agent_requested", godot::PropertyInfo(godot::Variant::OBJECT, "piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_properties", "game_properties"), &GamePiece::set_game_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_properties"), &GamePiece::get_game_properties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_properties", godot::PROPERTY_HINT_ARRAY_TYPE, "GameProperty"), "set_game_properties", "get_game_properties");

    godot::ClassDB::bind_method(godot::D_METHOD("set_actions", "actions"), &GamePiece::set_actions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_actions"), &GamePiece::get_actions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "actions", godot::PROPERTY_HINT_ARRAY_TYPE, "GamePieceAction"), "set_actions", "get_actions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_sub_game_pieces", "sub_game_pieces"), &GamePiece::set_sub_game_pieces);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sub_game_pieces"), &GamePiece::get_sub_game_pieces);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "sub_game_pieces", godot::PROPERTY_HINT_ARRAY_TYPE, "GamePiece"), "set_sub_game_pieces", "get_sub_game_pieces");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_sub_pieces", "max_sub_pieces"), &GamePiece::set_max_sub_pieces);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_sub_pieces"), &GamePiece::get_max_sub_pieces);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_sub_pieces"), "set_max_sub_pieces", "get_max_sub_pieces");

    // Expose Methods mimicking GDScript
    godot::ClassDB::bind_method(godot::D_METHOD("collect_actions"), &GamePiece::collect_actions);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_actions"), &GamePiece::clear_actions);
    godot::ClassDB::bind_method(godot::D_METHOD("add_action", "new_action"), &GamePiece::add_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_action_id_from_title", "title"), &GamePiece::get_action_id_from_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_action_id", "action"), &GamePiece::get_action_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_action", "action"), &GamePiece::has_action);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_action", "action"), &GamePiece::remove_action);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_action_at", "action_ID"), &GamePiece::remove_action_at);
    godot::ClassDB::bind_method(godot::D_METHOD("find_best_action", "instigating_action"), &GamePiece::find_best_action);
    godot::ClassDB::bind_method(godot::D_METHOD("take_action", "action_id"), &GamePiece::take_action);
    godot::ClassDB::bind_method(godot::D_METHOD("stop_acting", "action_id"), &GamePiece::stop_acting);
    godot::ClassDB::bind_method(godot::D_METHOD("get_action_value", "action_id"), &GamePiece::get_action_value);
    godot::ClassDB::bind_method(godot::D_METHOD("interact", "action_id"), &GamePiece::interact);
    godot::ClassDB::bind_method(godot::D_METHOD("join_interaction", "interaction"), &GamePiece::join_interaction);

    godot::ClassDB::bind_method(godot::D_METHOD("collect_properties"), &GamePiece::collect_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_properties"), &GamePiece::clear_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("add_property", "property"), &GamePiece::add_property);
    godot::ClassDB::bind_method(godot::D_METHOD("has_property", "property_name"), &GamePiece::has_property_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property", "property_name"), &GamePiece::get_property);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_id", "property_name"), &GamePiece::get_property_id);
    godot::ClassDB::bind_method(godot::D_METHOD("exhaust_property", "game_property"), &GamePiece::exhaust_property);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_property", "game_property"), &GamePiece::remove_property);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_property_at", "id"), &GamePiece::remove_property_at);

    godot::ClassDB::bind_method(godot::D_METHOD("find_sub_piece", "on"), &GamePiece::find_sub_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("add_sub_piece", "sub_piece"), &GamePiece::add_sub_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sub_piece_id", "sub_piece"), &GamePiece::get_sub_piece_id);
    godot::ClassDB::bind_method(godot::D_METHOD("is_in_sub_piece_chain", "sub_piece"), &GamePiece::is_in_sub_piece_chain);
    godot::ClassDB::bind_method(godot::D_METHOD("can_accept_sub_piece", "sub"), &GamePiece::can_accept_sub_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_sub_piece", "sub_piece"), &GamePiece::remove_sub_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_sub_piece_at", "id"), &GamePiece::remove_sub_piece_at);
    godot::ClassDB::bind_method(godot::D_METHOD("find_and_remove_sub_piece", "on"), &GamePiece::find_and_remove_sub_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("request_agents"), &GamePiece::request_agents);
}

GamePiece::GamePiece() {}

GamePiece::~GamePiece() {}

void GamePiece::_ready() {
    GameEntity::_ready();
    
    if (actions.size() == 0) {
        collect_actions();
    }
    
    if (game_properties.size() == 0) {
        collect_properties();
    }
}

// Setters / Getters
void GamePiece::set_game_properties(const godot::TypedArray<GameProperty>& p_props) { 
    if (p_props == game_properties) return;
    game_properties = p_props; 
}
godot::TypedArray<GameProperty> GamePiece::get_game_properties() const { return game_properties; }

void GamePiece::set_actions(const godot::TypedArray<GamePieceAction>& p_actions) { 
    if (p_actions == actions) return;
    actions = p_actions; 
}
godot::TypedArray<GamePieceAction> GamePiece::get_actions() const { return actions; }

void GamePiece::set_sub_game_pieces(const godot::TypedArray<GamePiece>& p_sub_pieces) { 
    if (p_sub_pieces == sub_game_pieces) return;
    sub_game_pieces = p_sub_pieces; 
}
godot::TypedArray<GamePiece> GamePiece::get_sub_game_pieces() const { return sub_game_pieces; }

void GamePiece::set_max_sub_pieces(int p_max) { 
    if (p_max == max_sub_pieces) return;
    max_sub_pieces = p_max; 
}
int GamePiece::get_max_sub_pieces() const { return max_sub_pieces; }


// USE FUNCTIONS
void GamePiece::collect_actions() {
    if (actions.size() > 0) clear_actions();
    
    if (has_node("Actions")) {
        godot::Node* act_node = get_node<godot::Node>("Actions");
        godot::TypedArray<godot::Node> children = act_node->get_children();
        for (int i = 0; i < children.size(); ++i) {
            GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(children[i]);
            if (action) actions.append(action);
        }
    }
}

void GamePiece::clear_actions() {
    actions.clear();
}

int GamePiece::add_action(GamePieceAction* new_action) {
    if (!new_action) return -1;
    int id = get_action_id(new_action);
    
    if (id < 0) {
        id = actions.size();
        
        if (has_node("Actions")) {
            godot::Node* act_node = get_node<godot::Node>("Actions");
            if (new_action->get_parent()) {
                new_action->get_parent()->remove_child(new_action);
            }
            act_node->add_child(new_action);
            
            actions.append(new_action);
            emit_signal("action_added", new_action);
        }
    }
    return id;
}

int GamePiece::get_action_id_from_title(const godot::String& title) const {
    for (int i = 0; i < actions.size(); ++i) {
        GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[i]);
        if (action && action->get_name() == title) {
            return i;
        }
    }
    return -1;
}

int GamePiece::get_action_id(GamePieceAction* action) const {
    return actions.find(action);
}

bool GamePiece::has_action(GamePieceAction* action) const {
    return get_action_id(action) >= 0;
}

bool GamePiece::remove_action(GamePieceAction* action) {
    int id = get_action_id(action);
    if (id >= 0) return remove_action_at(id);
    return false;
}

bool GamePiece::remove_action_at(int action_ID) {
    if (action_ID < 0 || action_ID >= actions.size()) return false;
    
    GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[action_ID]);
    actions.remove_at(action_ID);
    if (action) emit_signal("action_removed", action);
    
    return true;
}

godot::Array GamePiece::find_best_action(GamePieceAction* instigating_action) {
    int action_id = -1;
    int highest_score = -1;
    bool competing = false;
    
    if (!instigating_action) return godot::Array::make(action_id, competing);
    
    godot::TypedArray<godot::StringName> inst_groups = instigating_action->get_groups();
    
    for (int i = 0; i < actions.size(); ++i) {
        GamePieceAction* act = godot::Object::cast_to<GamePieceAction>(actions[i]);
        if (!act) continue;
        
        // Note: Assumes GamePieceAction exposes ActionStatus enum and get_status()
        if (act->get_status() != ActionStatus::IDLE) continue;
        
        int act_score = 0;
        bool act_competing = false;
        
        // Note: Assumes get_game_properties() is exposed
        godot::TypedArray<GameProperty> act_props = act->get_game_properties();
        for (int j = 0; j < act_props.size(); ++j) {
            GameProperty* p = godot::Object::cast_to<GameProperty>(act_props[j]);
            if (p) {
                if (p->get_locked()) act_score -= 1;
                else if (p->get_is_exhausted()) act_score -= 1; // Assuming is_exhausted() is bound
            }
        }
        
        for (int g = 0; g < inst_groups.size(); ++g) {
            godot::StringName group = inst_groups[g];
            if (act->is_in_group(group)) {
                act_score += 1;
            }
            
            // Note: Assumes get_competes_with_groups() is bound
            godot::TypedArray<godot::StringName> competes = act->get_competes_with_groups();
            if (competes.has(group)) {
                act_score += 1;
                act_competing = true;
            }
        }
        
        if (act_score > highest_score) {
            action_id = i;
            highest_score = act_score;
            competing = act_competing;
        }
    }
    
    return godot::Array::make(action_id, competing);
}

void GamePiece::take_action(int action_id) {
    if (action_id < 0 || action_id >= actions.size()) return;
    
    GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[action_id]);
    if (!action) return;
    
    if (action->get_status() == ActionStatus::IN_PROGRESS) return;
    
    int action_value = get_action_value(action_id);
    
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub_piece = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (!sub_piece) continue;
        
        int sub_id = sub_piece->get_action_id_from_title(action->get_name());
        if (sub_id < 0) continue;
        
        sub_piece->take_action(sub_id);
    }
    
    action->start_action();
}

void GamePiece::stop_acting(int action_id) {
    if (action_id < 0 || action_id >= actions.size()) return;
    
    GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[action_id]);
    if (!action) return;
    
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub_piece = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (!sub_piece) continue;
        
        int sub_id = sub_piece->get_action_id_from_title(action->get_name());
        if (sub_id < 0) continue;
        
        sub_piece->stop_acting(sub_id);
    }
    
    action->end_action();
}

int GamePiece::get_action_value(int action_id) {
    if (action_id < 0 || action_id >= actions.size()) return 0;
    
    GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[action_id]);
    if (!action) return 0;
    
    int action_value = action->get_action_value();
    
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub_piece = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (!sub_piece) continue;
        
        int sub_id = sub_piece->get_action_id_from_title(action->get_name());
        if (sub_id < 0) continue;
        
        int sub_value = sub_piece->get_action_value(sub_id);
        action_value += sub_value;
    }
    
    return action_value;
}

GameInteraction* GamePiece::interact(int action_id) {
    Game* game_instance = get_game();
    if (!game_instance) return nullptr;
    
    GameInteraction* interaction = game_instance->request_game_interaction();
    if (!interaction) return nullptr;
    
    godot::Array participation;
    participation.push_back(this);
    participation.push_back(action_id);
    
    interaction->set_instigator(participation);
    return interaction;
}

void GamePiece::join_interaction(GameInteraction* interaction) {
    if (!interaction) return;
    
    godot::Array instigator = interaction->get_instigator();
    if (instigator.size() < 2) return;
    
    GamePiece* inst_piece = godot::Object::cast_to<GamePiece>(instigator[0]);
    if (!inst_piece) return;
    
    int inst_action_id = instigator[1];
    godot::TypedArray<GamePieceAction> inst_actions = inst_piece->get_actions();
    if (inst_action_id < 0 || inst_action_id >= inst_actions.size()) return;
    
    GamePieceAction* inst_action = godot::Object::cast_to<GamePieceAction>(inst_actions[inst_action_id]);
    
    godot::Array do_act = find_best_action(inst_action);
    int action_id = do_act[0];
    bool competing = do_act[1];
    
    if (action_id >= 0) {
        godot::Array participation;
        participation.push_back(this);
        participation.push_back(action_id);
        
        if (competing) {
            // Note: Assuming GameInteraction exposes competition array manipulation
            godot::Array comp = interaction->get_competition();
            comp.push_back(participation);
            interaction->set_competition(comp);
        } else {
            // Note: Assuming GameInteraction exposes cooperation array manipulation
            godot::Array coop = interaction->get_cooperation();
            coop.push_back(participation);
            interaction->set_cooperation(coop);
        }
    }
}

// PROPERTY FUNCTIONS
void GamePiece::collect_properties() {
    if (game_properties.size() > 0) clear_properties();
    
    if (has_node("Properties")) {
        godot::Node* p = get_node<godot::Node>("Properties");
        godot::TypedArray<godot::Node> children = p->get_children();
        for (int i = 0; i < children.size(); ++i) {
            GameProperty* prop = godot::Object::cast_to<GameProperty>(children[i]);
            if (prop) {
                game_properties.append(prop);
                if (!prop->is_connected("value_exhausted", godot::Callable(this, "exhaust_property"))) {
                    prop->connect("value_exhausted", godot::Callable(this, "exhaust_property"));
                }
            }
        }
    }
}

void GamePiece::clear_properties() {
    game_properties.clear();
}

int GamePiece::add_property(GameProperty* property) {
    if (!property) return -1;
    
    int id = game_properties.find(property);
    if (id < 0) {
        id = game_properties.size();
        game_properties.append(property);
    }
    
    for (int i = 0; i < actions.size(); ++i) {
        GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[i]);
        if (action) action->restore_property(property);
    }
    
    return id;
}

bool GamePiece::has_property_name(const godot::String& property_name) const {
    for (int i = 0; i < game_properties.size(); ++i) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i]);
        if (prop && prop->get_name() == property_name) return true;
    }
    return false;
}

GameProperty* GamePiece::get_property(const godot::String& property_name) const {
    int property_id = get_property_id(property_name);
    if (property_id >= 0) {
        return godot::Object::cast_to<GameProperty>(game_properties[property_id]);
    }
    return nullptr;
}

int GamePiece::get_property_id(const godot::String& property_name) const {
    for (int i = 0; i < game_properties.size(); ++i) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i]);
        if (prop && prop->get_name() == property_name) return i;
    }
    return -1;
}

void GamePiece::exhaust_property(GameProperty* game_property) {
    int id = game_properties.find(game_property);
    if (id >= 0) {
        for (int i = 0; i < actions.size(); ++i) {
            GamePieceAction* gpa = godot::Object::cast_to<GamePieceAction>(actions[i]);
            if (!gpa) continue;
            
            // Note: Direct array member access per GDScript `gpa.gamePropertyIDs`
            // Assuming getter exists or mapped equivalent. For now, pseudo-mapping:
            godot::TypedArray<int> gpids = gpa->get("gamePropertyIDs"); 
            for (int j = 0; j < gpids.size(); ++j) {
                if (static_cast<int>(gpids[j]) == id) {
                    gpa->exhaust_property(j);
                }
            }
        }
    }
}

bool GamePiece::remove_property(GameProperty* game_property) {
    if (!game_property) return false;
    int id = get_property_id(game_property->get_name());
    return remove_property_at(id);
}

bool GamePiece::remove_property_at(int id) {
    if (id < 0 || id >= game_properties.size()) return false;
    
    GameProperty* property = godot::Object::cast_to<GameProperty>(game_properties[id]);
    game_properties.remove_at(id);
    
    for (int i = 0; i < actions.size(); ++i) {
        GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[i]);
        if (action) action->remove_property(property);
    }
    
    return true;
}

// SUB-PIECE FUNCTIONS
void GamePiece::find_sub_piece(godot::Variant on) {
    godot::Object* obj = on.operator godot::Object*();
    if (!obj) return;
    
    godot::Node* n = godot::Object::cast_to<godot::Node>(obj);
    if (!n) return;
    
    godot::TypedArray<godot::Node> gps = n->find_children("*", "GamePiece", true, false);
    if (gps.size() > 0) {
        GamePiece* new_piece = godot::Object::cast_to<GamePiece>(gps[0]);
        if (new_piece) add_sub_piece(new_piece);
    }
}

int GamePiece::add_sub_piece(GamePiece* sub_piece) {
    if (!sub_piece) return -1;
    
    int id = get_sub_piece_id(sub_piece);
    if (id < 0) {
        if (!can_accept_sub_piece(sub_piece)) return id;
        
        id = sub_game_pieces.size();
        sub_game_pieces.append(sub_piece);
        emit_signal("sub_piece_added", sub_piece);
    }
    return id;
}

int GamePiece::get_sub_piece_id(const GamePiece* sub_piece) const {
    return sub_game_pieces.find(sub_piece);
}

bool GamePiece::is_in_sub_piece_chain(const GamePiece* sub_piece) const {
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sp = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (!sp) continue;
        if (sp == sub_piece) return true;
        if (sp->is_in_sub_piece_chain(sub_piece)) return true;
    }
    return false;
}

bool GamePiece::can_accept_sub_piece(const GamePiece* sub) const {
    if (max_sub_pieces > 0 && sub_game_pieces.size() >= max_sub_pieces) return false;
    if (is_in_sub_piece_chain(sub)) return false;
    return true;
}

bool GamePiece::remove_sub_piece(GamePiece* sub_piece) {
    return remove_sub_piece_at(get_sub_piece_id(sub_piece));
}

bool GamePiece::remove_sub_piece_at(int id) {
    if (id < 0 || id >= sub_game_pieces.size()) return false;
    
    GamePiece* sb = godot::Object::cast_to<GamePiece>(sub_game_pieces[id]);
    sub_game_pieces.remove_at(id);
    if (sb) emit_signal("sub_piece_removed", sb);
    
    return true;
}

void GamePiece::find_and_remove_sub_piece(godot::Variant on) {
    godot::Object* obj = on.operator godot::Object*();
    if (!obj) return;
    
    godot::Node* n = godot::Object::cast_to<godot::Node>(obj);
    if (!n) return;
    
    godot::TypedArray<godot::Node> gps = n->find_children("*", "GamePiece", true, false);
    if (gps.size() > 0) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(gps[0]);
        if (piece) remove_sub_piece(piece);
    }
}

godot::TypedArray<GameAgent> GamePiece::request_agents() {
    requested_agents.clear();
    emit_signal("agent_requested", this);
    
    godot::TypedArray<GameAgent> r_a = requested_agents;
    requested_agents.clear();
    return r_a;
}

// OVERRIDES
void GamePiece::enter_game() {
    GameEntity::enter_game();
    
    Game* game_instance = get_game();
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (sub) sub->set_game(game_instance);
    }
}

void GamePiece::game_pause() {
    if (get_entity_is_paused()) return;
    
    GameEntity::game_pause();
    
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (sub) sub->game_pause();
    }
}

void GamePiece::game_continue() {
    if (!get_entity_is_paused()) return;
    
    GameEntity::game_continue();
    
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (sub) sub->game_continue();
    }
}

void GamePiece::exit_game() {
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (sub) sub->exit_game();
    }
    
    GameEntity::exit_game();
}

void GamePiece::game_process(double delta) {
    if (game_processed) return;
    
    double piece_delta = delta * get_time_scale();
    
    for (int i = 0; i < game_properties.size(); ++i) {
        GameProperty* game_property = godot::Object::cast_to<GameProperty>(game_properties[i]);
        if (game_property) game_property->game_process(piece_delta);
    }
    
    for (int i = 0; i < actions.size(); ++i) {
        GamePieceAction* action = godot::Object::cast_to<GamePieceAction>(actions[i]);
        if (action && action->get_status() == ActionStatus::IN_PROGRESS) {
            int oldValue = action->get_action_value();
            int newValue = get_action_value(i);
            int diff = newValue - oldValue;
            action->update_action(piece_delta, diff);
        }
    }
    
    game_processed = true;
}

void GamePiece::game_process_clear() {
    for (int i = 0; i < game_properties.size(); ++i) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i]);
        if (prop) prop->game_process_clear();
    }
    game_processed = false;
}

void GamePiece::action_consequences(int score, const godot::Dictionary& consequences) {
    GameEntity::action_consequences(score, consequences);
    
    if (consequences.has("add_action")) {
        godot::TypedArray<GamePieceAction> actions_to_add = consequences["add_action"];
        for (int i = 0; i < actions_to_add.size(); ++i) {
            add_action(godot::Object::cast_to<GamePieceAction>(actions_to_add[i]));
        }
    }
    
    if (consequences.has("remove_action")) {
        godot::TypedArray<GamePieceAction> actions_to_remove = consequences["remove_action"];
        for (int i = 0; i < actions_to_remove.size(); ++i) {
            remove_action(godot::Object::cast_to<GamePieceAction>(actions_to_remove[i]));
        }
    }
    
    if (consequences.has("add_sub_piece")) {
        godot::TypedArray<GamePiece> subs_to_add = consequences["add_sub_piece"];
        for (int i = 0; i < subs_to_add.size(); ++i) {
            add_sub_piece(godot::Object::cast_to<GamePiece>(subs_to_add[i]));
        }
    }
    
    if (consequences.has("remove_sub_piece")) {
        godot::TypedArray<GamePiece> subs_to_remove = consequences["remove_sub_piece"];
        for (int i = 0; i < subs_to_remove.size(); ++i) {
            remove_sub_piece(godot::Object::cast_to<GamePiece>(subs_to_remove[i]));
        }
    }
    
    if (consequences.has("sub-game_pieces")) {
        godot::Dictionary sub_dict = consequences["sub-game_pieces"];
        for (int i = 0; i < sub_game_pieces.size(); ++i) {
            GamePiece* piece = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
            if (piece) piece->action_consequences(score, sub_dict);
        }
    }
    
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sub = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (sub) {
            godot::String sub_setting = "sub-game_piece_" + sub->get_name();
            if (consequences.has(sub_setting)) {
                sub->action_consequences(score, consequences[sub_setting]);
            }
        }
    }
    
    if (consequences.has("add_property")) {
        godot::TypedArray<GameProperty> props_to_add = consequences["add_property"];
        for (int i = 0; i < props_to_add.size(); ++i) {
            add_property(godot::Object::cast_to<GameProperty>(props_to_add[i]));
        }
    }
    
    if (consequences.has("remove_property")) {
        godot::TypedArray<GameProperty> props_to_remove = consequences["remove_property"];
        for (int i = 0; i < props_to_remove.size(); ++i) {
            remove_property(godot::Object::cast_to<GameProperty>(props_to_remove[i]));
        }
    }
    
    if (consequences.has("properties")) {
        godot::Dictionary prop_dict = consequences["properties"];
        for (int i = 0; i < game_properties.size(); ++i) {
            GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i]);
            if (prop) prop->action_consequences(score, prop_dict);
        }
    }
    
    for (int i = 0; i < game_properties.size(); ++i) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i]);
        if (prop) {
            godot::String prop_setting = "property_" + prop->get_name();
            if (consequences.has(prop_setting)) {
                prop->action_consequences(score, consequences[prop_setting]);
            }
        }
    }
}

godot::Dictionary GamePiece::save_data() const {
    godot::Dictionary data = GameEntity::save_data();
    
    if (game_properties.size() > 0) {
        godot::Array gpAr;
        for (int i = 0; i < game_properties.size(); ++i) {
            GameProperty* gp = godot::Object::cast_to<GameProperty>(game_properties[i]);
            if (gp) gpAr.append(gp->save_data());
        }
        data["game_properties"] = gpAr;
    }
    
    if (actions.size() > 0) {
        godot::Array gpuAr;
        for (int i = 0; i < actions.size(); ++i) {
            GamePieceAction* gpu = godot::Object::cast_to<GamePieceAction>(actions[i]);
            if (gpu) gpuAr.append(gpu->save_data()); // Assuming save_data on Action
        }
        data["actions"] = gpuAr;
    }
    
    data["max_sub_pieces"] = max_sub_pieces;
    return data;
}

void GamePiece::load_data(const godot::Dictionary& data) {
    GameEntity::load_data(data);
    
    if (data.has("game_properties")) {
        for (int i = 0; i < game_properties.size(); ++i) {
            GameProperty* gp = godot::Object::cast_to<GameProperty>(game_properties[i]);
            if (gp) gp->queue_free();
        }
        
        if (!has_node("Properties")) {
            godot::Node* pNode = memnew(godot::Node);
            pNode->set_name("Properties");
            add_child(pNode);
        }
        game_properties.clear();
        
        godot::Array gpAr = data["game_properties"];
        godot::Node* props_node = get_node<godot::Node>("Properties");
        
        for (int i = 0; i < gpAr.size(); ++i) {
            // Note: Assuming GameProperty instantiation 
            // GameProperty* property = memnew(GameProperty);
            // props_node->add_child(property);
            // property->load_data(gpAr[i]);
            // game_properties.append(property);
        }
    }
    
    if (data.has("actions")) {
        for (int i = 0; i < actions.size(); ++i) {
            GamePieceAction* gpu = godot::Object::cast_to<GamePieceAction>(actions[i]);
            if (gpu) gpu->queue_free();
        }
        
        if (!has_node("Actions")) {
            godot::Node* uNode = memnew(godot::Node);
            uNode->set_name("Actions");
            add_child(uNode);
        }
        actions.clear();
        
        godot::Array gpuAr = data["actions"];
        godot::Node* act_node = get_node<godot::Node>("Actions");
        
        for (int i = 0; i < gpuAr.size(); ++i) {
            // Note: Assuming GamePieceAction instantiation
            // GamePieceAction* action = memnew(GamePieceAction);
            // act_node->add_child(action);
            // action->load_data(gpuAr[i]);
            // actions.append(action);
        }
    }
    
    if (data.has("max_sub_pieces")) {
        max_sub_pieces = data["max_sub_pieces"];
    }
}

} // namespace ideam::godot_ext