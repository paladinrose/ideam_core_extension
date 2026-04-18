#include "game_piece.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

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
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_properties", "properties"), &GamePiece::set_game_properties);
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

    // Methods
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
    
    godot::ClassDB::bind_method(godot::D_METHOD("collect_properties"), &GamePiece::collect_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_properties"), &GamePiece::clear_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("add_property", "property"), &GamePiece::add_property);
    godot::ClassDB::bind_method(godot::D_METHOD("has_property", "property_name"), &GamePiece::has_property);
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

    if (godot::Engine::get_singleton()->is_editor_hint()) return;

    if (actions.size() == 0) collect_actions();
    if (game_properties.size() == 0) collect_properties();
}

// Getters / Setters
void GamePiece::set_game_properties(const godot::TypedArray<GameProperty>& p_properties) { game_properties = p_properties; }
godot::TypedArray<GameProperty> GamePiece::get_game_properties() const { return game_properties; }
void GamePiece::set_actions(const godot::TypedArray<GamePieceAction>& p_actions) { actions = p_actions; }
godot::TypedArray<GamePieceAction> GamePiece::get_actions() const { return actions; }
void GamePiece::set_sub_game_pieces(const godot::TypedArray<GamePiece>& p_pieces) { sub_game_pieces = p_pieces; }
godot::TypedArray<GamePiece> GamePiece::get_sub_game_pieces() const { return sub_game_pieces; }
void GamePiece::set_max_sub_pieces(int p_max) { max_sub_pieces = p_max; }
int GamePiece::get_max_sub_pieces() const { return max_sub_pieces; }

// Use Functions
void GamePiece::collect_actions() {
    clear_actions();
    if (has_node("Actions")) {
        godot::Node* act_node = get_node<godot::Node>("Actions");
        godot::TypedArray<godot::Node> children = act_node->get_children();
        for (int i = 0; i < children.size(); ++i) {
            if (godot::Object* child = children[i]) {
                if (child->get_class() == "GamePieceAction") { // Ensure correct class mapping
                    actions.append(child);
                }
            }
        }
    }
}

void GamePiece::clear_actions() { actions.clear(); }

int GamePiece::add_action(GamePieceAction* new_action) {
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
        godot::Object* act = actions[i];
        // Assuming action has a string name property map
        if (act && act->get("name") == title) return i; 
    }
    return -1;
}

int GamePiece::get_action_id(const GamePieceAction* action) const { return actions.find(action); }

bool GamePiece::has_action(const GamePieceAction* action) const { return get_action_id(action) >= 0; }

bool GamePiece::remove_action(GamePieceAction* action) {
    int id = get_action_id(action);
    return id >= 0 ? remove_action_at(id) : false;
}

bool GamePiece::remove_action_at(int action_ID) {
    if (action_ID < 0 || action_ID >= actions.size()) return false;
    godot::Object* action = actions[action_ID];
    actions.remove_at(action_ID);
    emit_signal("action_removed", action);
    return true;
}

godot::Array GamePiece::find_best_action(const GamePieceAction* instigating_action) const {
    godot::Array result;
    int action_id = -1;
    bool competing = false;
    // ... Implement logic checking action properties and groups
    result.push_back(action_id);
    result.push_back(competing);
    return result;
}

void GamePiece::take_action(int action_id) {
    if (action_id < 0 || action_id >= actions.size()) return;
    
    // Implement utilizing GamePieceAction API...
    // Call take_action sequentially on sub_pieces
}

void GamePiece::stop_acting(int action_id) {
    if (action_id < 0 || action_id >= actions.size()) return;
    // Iterate sub_pieces -> stop_acting
}

int GamePiece::get_action_value(int action_id) const {
    int val = 0;
    // val = action->get_action_value();
    // val += sub_piece->get_action_value();
    return val;
}

GameInteraction* GamePiece::interact(int action_id) { return nullptr; }
void GamePiece::join_interaction(GameInteraction* interaction) {}

// Property Functions
void GamePiece::collect_properties() {
    clear_properties();
    if (has_node("Properties")) {
        godot::Node* p = get_node<godot::Node>("Properties");
        godot::TypedArray<godot::Node> children = p->get_children();
        for (int i = 0; i < children.size(); ++i) {
            if (godot::Object* child = children[i]) {
                if (child->get_class() == "GameProperty") {
                    game_properties.append(child);
                    // if (!child->is_connected(...)) child->connect("value_exhausted", callable)
                }
            }
        }
    }
}

void GamePiece::clear_properties() { game_properties.clear(); }

int GamePiece::add_property(GameProperty* property) {
    int id = game_properties.find(property);
    if (id < 0) {
        id = game_properties.size();
        game_properties.append(property);
    }
    // Update actions...
    return id;
}

bool GamePiece::has_property(const godot::String& property_name) const {
    return get_property_id(property_name) >= 0;
}

GameProperty* GamePiece::get_property(const godot::String& property_name) const {
    int id = get_property_id(property_name);
    return id >= 0 ? godot::Object::cast_to<GameProperty>(game_properties[id]) : nullptr;
}

int GamePiece::get_property_id(const godot::String& property_name) const {
    for (int i = 0; i < game_properties.size(); ++i) {
        godot::Object* prop = game_properties[i];
        if (prop && prop->get("name") == property_name) return i;
    }
    return -1;
}

void GamePiece::exhaust_property(GameProperty* game_property) {
    int id = game_properties.find(game_property);
    if (id >= 0) {
        // notify actions...
    }
}

bool GamePiece::remove_property(GameProperty* game_property) {
    // Requires name extraction
    return false; 
}

bool GamePiece::remove_property_at(int id) {
    if (id < 0 || id >= game_properties.size()) return false;
    game_properties.remove_at(id);
    // notify actions...
    return true;
}

// Sub-Piece Functions
void GamePiece::find_sub_piece(godot::Variant on) {
    // Use engine API to find_children("GamePiece")
}

int GamePiece::add_sub_piece(GamePiece* sub_piece) {
    int id = get_sub_piece_id(sub_piece);
    if (id < 0) {
        if (!can_accept_sub_piece(sub_piece)) return id;
        id = sub_game_pieces.size();
        sub_game_pieces.append(sub_piece);
        emit_signal("sub_piece_added", sub_piece);
    }
    return id;
}

int GamePiece::get_sub_piece_id(const GamePiece* sub_piece) const { return sub_game_pieces.find(sub_piece); }

bool GamePiece::is_in_sub_piece_chain(const GamePiece* sub_piece) const {
    for (int i = 0; i < sub_game_pieces.size(); ++i) {
        GamePiece* sp = godot::Object::cast_to<GamePiece>(sub_game_pieces[i]);
        if (sp == sub_piece) return true;
        if (sp && sp->is_in_sub_piece_chain(sub_piece)) return true;
    }
    return false;
}

bool GamePiece::can_accept_sub_piece(const GamePiece* sub) const {
    if (max_sub_pieces > 0 && sub_game_pieces.size() >= max_sub_pieces) return false;
    if (sub && sub->is_in_sub_piece_chain(this)) return false;
    return true;
}

bool GamePiece::remove_sub_piece(GamePiece* sub_piece) { return remove_sub_piece_at(get_sub_piece_id(sub_piece)); }

bool GamePiece::remove_sub_piece_at(int id) {
    if (id < 0 || id >= sub_game_pieces.size()) return false;
    godot::Object* sb = sub_game_pieces[id];
    sub_game_pieces.remove_at(id);
    emit_signal("sub_piece_removed", sb);
    return true;
}

void GamePiece::find_and_remove_sub_piece(godot::Variant on) {}

godot::TypedArray<GameAgent> GamePiece::request_agents() {
    requested_agents.clear();
    emit_signal("agent_requested", this);
    godot::TypedArray<GameAgent> r_a = requested_agents;
    requested_agents.clear();
    return r_a;
}

// Function Overrides
void GamePiece::enter_game() {
    GameEntity::enter_game();
    // for sub in sub_game_pieces -> sub.set_game(_game)
}

void GamePiece::game_pause() {
    // if entity_is_paused return;
    GameEntity::game_pause();
    // for sub in sub_game_pieces -> sub.game_pause()
}

void GamePiece::game_continue() {
    // if not entity_is_paused return;
    GameEntity::game_continue();
    // for sub in sub_game_pieces -> sub.game_continue()
}

void GamePiece::exit_game() {
    // for sub in sub_game_pieces -> sub.exit_game()
    GameEntity::exit_game();
}

void GamePiece::game_process(double delta) {
    // if (game_processed) return;
    // double piece_delta = delta * time_scale;
    // Process properties and actions
    // game_processed = true;
}

void GamePiece::game_process_clear() {
    // Clear prop processing
    // game_processed = false;
}

void GamePiece::action_consequences(int score, const godot::Dictionary& consequences) {
    GameEntity::action_consequences(score, consequences);
    // Parse consequences...
    
    // DOD NOTE: Refactor JSON/Dictionary consequences payloads into tightly packed bitmasks 
    // or enums passed directly to hardware registers. Avoid variant type checking on the hotpath.
}

godot::Dictionary GamePiece::save_data() const {
    godot::Dictionary data = GameEntity::save_data();
    // serialize arrays
    return data;
}

void GamePiece::load_data(const godot::Dictionary& data) {
    GameEntity::load_data(data);
    // deserialize arrays
}

} // namespace ideam::godot_ext