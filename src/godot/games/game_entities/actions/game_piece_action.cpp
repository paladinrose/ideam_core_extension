#include "game_piece_action.h"
#include <godot_cpp/core/class_db.hpp>

// Assuming existence in project structure per instructions
#include "../../game_property.h"

namespace ideam::godot_ext {

void GamePieceAction::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("action_started"));
    ADD_SIGNAL(godot::MethodInfo("action_already_started"));
    ADD_SIGNAL(godot::MethodInfo("action_updated"));
    ADD_SIGNAL(godot::MethodInfo("action_refreshed"));
    ADD_SIGNAL(godot::MethodInfo("action_interrupted", godot::PropertyInfo(godot::Variant::OBJECT, "interrupter", godot::PROPERTY_HINT_NODE_TYPE, "GamePieceAction")));
    ADD_SIGNAL(godot::MethodInfo("action_ended"));
    ADD_SIGNAL(godot::MethodInfo("action_succeeded"));
    ADD_SIGNAL(godot::MethodInfo("action_failed"));

    // Enums
    BIND_ENUM_CONSTANT(ActionStatus::IDLE);
    BIND_ENUM_CONSTANT(ActionStatus::STARTED);
    BIND_ENUM_CONSTANT(ActionStatus::IN_PROGRESS);
    BIND_ENUM_CONSTANT(ActionStatus::INTERRUPTED);
    BIND_ENUM_CONSTANT(ActionStatus::SUCCESSFUL);
    BIND_ENUM_CONSTANT(ActionStatus::FAILED);

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_lock_piece", "lock"), &GamePieceAction::set_lock_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("get_lock_piece"), &GamePieceAction::get_lock_piece);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "lock_piece"), "set_lock_piece", "get_lock_piece");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_properties", "properties"), &GamePieceAction::set_game_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_properties"), &GamePieceAction::get_game_properties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_properties", godot::PROPERTY_HINT_ARRAY_TYPE, "GameProperty"), "set_game_properties", "get_game_properties");

    godot::ClassDB::bind_method(godot::D_METHOD("set_property_locks", "locks"), &GamePieceAction::set_property_locks);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_locks"), &GamePieceAction::get_property_locks);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "property_locks", godot::PROPERTY_HINT_ARRAY_TYPE, "bool"), "set_property_locks", "get_property_locks");

    godot::ClassDB::bind_method(godot::D_METHOD("set_property_use_values", "values"), &GamePieceAction::set_property_use_values);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_use_values"), &GamePieceAction::get_property_use_values);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "property_use_values", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_property_use_values", "get_property_use_values");

    godot::ClassDB::bind_method(godot::D_METHOD("set_refresh_time", "time"), &GamePieceAction::set_refresh_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_refresh_time"), &GamePieceAction::get_refresh_time);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "refresh_time"), "set_refresh_time", "get_refresh_time");

    godot::ClassDB::bind_method(godot::D_METHOD("set_competes_with_groups", "groups"), &GamePieceAction::set_competes_with_groups);
    godot::ClassDB::bind_method(godot::D_METHOD("get_competes_with_groups"), &GamePieceAction::get_competes_with_groups);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "competes_with_groups", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_competes_with_groups", "get_competes_with_groups");

    godot::ClassDB::bind_method(godot::D_METHOD("set_status", "status"), &GamePieceAction::set_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_status"), &GamePieceAction::get_status);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "status", godot::PROPERTY_HINT_ENUM, "Idle,Started,InProgress,Interrupted,Successful,Failed"), "set_status", "get_status");

    godot::ClassDB::bind_method(godot::D_METHOD("set_success_consequences", "consequences"), &GamePieceAction::set_success_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("get_success_consequences"), &GamePieceAction::get_success_consequences);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "success_consequences"), "set_success_consequences", "get_success_consequences");

    godot::ClassDB::bind_method(godot::D_METHOD("set_failure_consequences", "consequences"), &GamePieceAction::set_failure_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("get_failure_consequences"), &GamePieceAction::get_failure_consequences);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "failure_consequences"), "set_failure_consequences", "get_failure_consequences");

    godot::ClassDB::bind_method(godot::D_METHOD("set_interruption_consequences", "consequences"), &GamePieceAction::set_interruption_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("get_interruption_consequences"), &GamePieceAction::get_interruption_consequences);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "interruption_consequences"), "set_interruption_consequences", "get_interruption_consequences");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("gather_game_property_titles"), &GamePieceAction::gather_game_property_titles);
    godot::ClassDB::bind_method(godot::D_METHOD("add_property", "property", "use_value", "lock"), &GamePieceAction::add_property, DEFVAL(0), DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("exhaust_property", "local_property_id"), &GamePieceAction::exhaust_property);
    godot::ClassDB::bind_method(godot::D_METHOD("restore_property", "property"), &GamePieceAction::restore_property);
    godot::ClassDB::bind_method(godot::D_METHOD("has_property", "property_name"), &GamePieceAction::has_property);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property", "property_name"), &GamePieceAction::get_property);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_id", "property_name"), &GamePieceAction::get_property_id);
    godot::ClassDB::bind_method(godot::D_METHOD("missing_properties"), &GamePieceAction::missing_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_property", "game_property"), &GamePieceAction::remove_property);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_property_at", "id"), &GamePieceAction::remove_property_at);
    
    godot::ClassDB::bind_method(godot::D_METHOD("get_action_value"), &GamePieceAction::get_action_value);
    godot::ClassDB::bind_method(godot::D_METHOD("start_action"), &GamePieceAction::start_action);
    godot::ClassDB::bind_method(godot::D_METHOD("refresh_action"), &GamePieceAction::refresh_action);
    godot::ClassDB::bind_method(godot::D_METHOD("update_action", "delta", "valueChange"), &GamePieceAction::update_action);
    godot::ClassDB::bind_method(godot::D_METHOD("stop_action"), &GamePieceAction::stop_action);
    godot::ClassDB::bind_method(godot::D_METHOD("interrupt_action", "interrupter"), &GamePieceAction::interrupt_action);
    godot::ClassDB::bind_method(godot::D_METHOD("end_action"), &GamePieceAction::end_action);
    godot::ClassDB::bind_method(godot::D_METHOD("action_success"), &GamePieceAction::action_success);
    godot::ClassDB::bind_method(godot::D_METHOD("action_failure"), &GamePieceAction::action_failure);
    
    godot::ClassDB::bind_method(godot::D_METHOD("action_consequences", "score", "consequences"), &GamePieceAction::action_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("save_data"), &GamePieceAction::save_data);
    godot::ClassDB::bind_method(godot::D_METHOD("load_data", "data"), &GamePieceAction::load_data);
}

GamePieceAction::GamePieceAction() {}

GamePieceAction::~GamePieceAction() {}

// Setters / Getters
void GamePieceAction::set_lock_piece(bool p_lock) { lock_piece = p_lock; }
bool GamePieceAction::get_lock_piece() const { return lock_piece; }

void GamePieceAction::set_game_properties(const godot::TypedArray<GameProperty>& p_properties) { game_properties = p_properties; }
godot::TypedArray<GameProperty> GamePieceAction::get_game_properties() const { return game_properties; }

void GamePieceAction::set_property_locks(const godot::TypedArray<bool>& p_locks) { property_locks = p_locks; }
godot::TypedArray<bool> GamePieceAction::get_property_locks() const { return property_locks; }

void GamePieceAction::set_property_use_values(const godot::TypedArray<int>& p_values) { property_use_values = p_values; }
godot::TypedArray<int> GamePieceAction::get_property_use_values() const { return property_use_values; }

void GamePieceAction::set_refresh_time(float p_time) { refresh_time = p_time; }
float GamePieceAction::get_refresh_time() const { return refresh_time; }

void GamePieceAction::set_competes_with_groups(const godot::TypedArray<godot::String>& p_groups) { competes_with_groups = p_groups; }
godot::TypedArray<godot::String> GamePieceAction::get_competes_with_groups() const { return competes_with_groups; }

void GamePieceAction::set_status(ActionStatus p_status) { status = p_status; }
ActionStatus GamePieceAction::get_status() const { return status; }

void GamePieceAction::set_success_consequences(const godot::Dictionary& p_consequences) { success_consequences = p_consequences; }
godot::Dictionary GamePieceAction::get_success_consequences() const { return success_consequences; }

void GamePieceAction::set_failure_consequences(const godot::Dictionary& p_consequences) { failure_consequences = p_consequences; }
godot::Dictionary GamePieceAction::get_failure_consequences() const { return failure_consequences; }

void GamePieceAction::set_interruption_consequences(const godot::Dictionary& p_consequences) { interruption_consequences = p_consequences; }
godot::Dictionary GamePieceAction::get_interruption_consequences() const { return interruption_consequences; }


// Class Functions
godot::TypedArray<godot::String> GamePieceAction::gather_game_property_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < game_properties.size(); ++i) {
        if (GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i])) {
            names.append(prop->get_title()); 
        }
    }
    return names;
}

int GamePieceAction::add_property(GameProperty* property, int use_value, bool lock) {
    int id = game_properties.find(property);
    if (id < 0) {
        id = game_properties.size();
        game_properties.append(property);
        property_locks.append(lock);
        property_use_values.append(use_value);
    }
    return id;
}

void GamePieceAction::exhaust_property(int local_property_id) {
    if (local_property_id < 0 || local_property_id >= game_properties.size()) return;
    if (status == ActionStatus::IN_PROGRESS) {
        action_failure();
    }
}

int GamePieceAction::restore_property(GameProperty* property) {
    int id = game_properties.find(property);
    if (id < 0 && property) {
        godot::String prop_name = property->get_name();
        if (missing_property_ids.has(prop_name)) {
            id = missing_property_ids[prop_name];
            game_properties[id] = property;
            missing_property_ids.erase(prop_name);
        }
    }
    return id;
}

bool GamePieceAction::has_property(const godot::String& property_name) const {
    return get_property_id(property_name) >= 0;
}

GameProperty* GamePieceAction::get_property(const godot::String& property_name) const {
    int id = get_property_id(property_name);
    return id >= 0 ? godot::Object::cast_to<GameProperty>(game_properties[id]) : nullptr;
}

int GamePieceAction::get_property_id(const godot::String& property_name) const {
    for (int i = 0; i < game_properties.size(); ++i) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[i]);
        if (prop && prop->get_name() == property_name) return i;
    }
    return -1;
}

bool GamePieceAction::missing_properties() const {
    for (int i = 0; i < game_properties.size(); ++i) {
        if (!godot::Object::cast_to<GameProperty>(game_properties[i])) {
            return true;
        }
    }
    return false;
}

bool GamePieceAction::remove_property(GameProperty* game_property) {
    if (!game_property) return false;
    return remove_property_at(get_property_id(game_property->get_name()));
}

bool GamePieceAction::remove_property_at(int id) {
    if (id < 0 || id >= game_properties.size()) return false;
    
    GameProperty* newly_missing = godot::Object::cast_to<GameProperty>(game_properties[id]);
    if (newly_missing) {
        missing_property_ids[newly_missing->get_name()] = id;
    }
    
    game_properties[id] = godot::Variant();
    return true;
}

int GamePieceAction::get_action_value() {
    if (refresh_properties) {
        refresh_action();
    }
    return current_value;
}

void GamePieceAction::start_action() {
    if (status != ActionStatus::IDLE) {
        emit_signal("action_already_started");
        return;
    }
    
    if (missing_properties()) return;
    
    for (int p = 0; p < game_properties.size(); ++p) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[p]);
        if (prop && prop->get_locked()) return;
    }
    
    for (int p = 0; p < game_properties.size(); ++p) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[p]);
        if (prop && static_cast<bool>(property_locks[p])) {
            prop->set_locked(true);
        }
    }
    
    refresh_properties = true;
    refresh_action();
    
    current_time = 0.0f;
    status = ActionStatus::IN_PROGRESS;
    emit_signal("action_started");
}

void GamePieceAction::refresh_action() {
    if (!refresh_properties) return;
    
    current_value = value;
    
    for (int p = 0; p < game_properties.size(); ++p) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[p]);
        if (prop) {
            int use_val = property_use_values[p];
            current_value += static_cast<int>(prop->use_as_resource(use_val));
        }
    }
    
    refresh_properties = false;
    emit_signal("action_refreshed");
}

void GamePieceAction::update_action(double delta, int valueChange) {
    current_value += valueChange;
    current_time += delta;
    
    if (refresh_time > 0.0f) {
        while (current_time >= refresh_time) {
            current_time -= refresh_time;
            refresh_properties = true;
        }
        refresh_action();
    }
    
    emit_signal("action_updated");
}

void GamePieceAction::stop_action() {
    if (status != ActionStatus::IN_PROGRESS) return;
    
    for (int p = 0; p < game_properties.size(); ++p) {
        GameProperty* prop = godot::Object::cast_to<GameProperty>(game_properties[p]);
        if (prop && static_cast<bool>(property_locks[p])) {
            prop->set_locked(false);
        }
    }
    
    status = ActionStatus::IDLE;
}

void GamePieceAction::interrupt_action(GamePieceAction* interrupter) {
    stop_action();
    status = ActionStatus::INTERRUPTED;
    emit_signal("action_interrupted", interrupter);
}

void GamePieceAction::end_action() {
    stop_action();
    status = ActionStatus::IDLE;
    emit_signal("action_ended");
}

void GamePieceAction::action_success() {
    stop_action();
    status = ActionStatus::SUCCESSFUL;
    emit_signal("action_succeeded");
}

void GamePieceAction::action_failure() {
    stop_action();
    status = ActionStatus::FAILED;
    emit_signal("action_failed");
}

void GamePieceAction::action_consequences(int score, const godot::Dictionary& consequences) {
    if (consequences.has("set_value")) {
        godot::Variant set_val = consequences["set_value"];
        if (set_val.get_type() == godot::Variant::STRING) {
            godot::String set_str = set_val;
            if (set_str == "score") value = score;
            else if (set_str == "-score") value = -score;
        } else {
            value = static_cast<int>(set_val);
        }
    }
    
    if (consequences.has("change_value")) {
        godot::Variant change_val = consequences["change_value"];
        if (change_val.get_type() == godot::Variant::STRING) {
            godot::String change_str = change_val;
            if (change_str == "score") value += score;
            else if (change_str == "-score") value -= score;
        } else {
            value += static_cast<int>(change_val);
        }
    }
}

godot::Dictionary GamePieceAction::save_data() const {
    godot::Dictionary data;
    data["property_use_values"] = property_use_values;
    data["refresh_time"] = refresh_time;
    data["refresh_properties"] = refresh_properties;
    data["status"] = status;
    data["value"] = value;
    return data;
}

void GamePieceAction::load_data(const godot::Dictionary& data) {
    if (data.has("property_use_values")) property_use_values = data["property_use_values"];
    if (data.has("refresh_time")) refresh_time = data["refresh_time"];
    if (data.has("refresh_properties")) refresh_properties = data["refresh_properties"];
    if (data.has("status")) status = static_cast<ActionStatus>(static_cast<int>(data["status"]));
    if (data.has("value")) value = data["value"];
}

} // namespace ideam::godot_ext