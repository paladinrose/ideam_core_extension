#include "game_property_modifier.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GamePropertyModifier::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("started_modifying"));
    ADD_SIGNAL(godot::MethodInfo("stopped_modifying"));
    ADD_SIGNAL(godot::MethodInfo("value_changed", godot::PropertyInfo(godot::Variant::INT, "value")));

    // Enums
    BIND_ENUM_CONSTANT(ModifierType::STATIC);
    BIND_ENUM_CONSTANT(ModifierType::TIMED);
    BIND_ENUM_CONSTANT(ModifierType::TIMED_DIMINISHING);
    BIND_ENUM_CONSTANT(ModifierType::RESOURCE);

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_type", "type"), &GamePropertyModifier::set_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type"), &GamePropertyModifier::get_type);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type", godot::PROPERTY_HINT_ENUM, "Static,Timed,Timed Diminishing,Resource"), "set_type", "get_type");

    godot::ClassDB::bind_method(godot::D_METHOD("set_value", "newValue"), &GamePropertyModifier::set_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_value"), &GamePropertyModifier::get_value);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "value"), "set_value", "get_value");

    godot::ClassDB::bind_method(godot::D_METHOD("set_time_limit", "time_limit"), &GamePropertyModifier::set_time_limit);
    godot::ClassDB::bind_method(godot::D_METHOD("get_time_limit"), &GamePropertyModifier::get_time_limit);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "time_limit"), "set_time_limit", "get_time_limit");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("modifier_start"), &GamePropertyModifier::modifier_start);
    godot::ClassDB::bind_method(godot::D_METHOD("game_process", "delta"), &GamePropertyModifier::game_process);
    godot::ClassDB::bind_method(godot::D_METHOD("game_process_clear"), &GamePropertyModifier::game_process_clear);
    godot::ClassDB::bind_method(godot::D_METHOD("modifier_stop"), &GamePropertyModifier::modifier_stop);
    
    godot::ClassDB::bind_method(godot::D_METHOD("save_data"), &GamePropertyModifier::save_data);
    godot::ClassDB::bind_method(godot::D_METHOD("load_data", "data"), &GamePropertyModifier::load_data);
}

GamePropertyModifier::GamePropertyModifier() {}
GamePropertyModifier::~GamePropertyModifier() {}

// Setters / Getters
void GamePropertyModifier::set_type(ModifierType p_type) { type = p_type; }
ModifierType GamePropertyModifier::get_type() const { return type; }

void GamePropertyModifier::set_value(int p_value) {
    if (p_value == _value) return;
    _value = p_value;
    emit_signal("value_changed", _value);
}
int GamePropertyModifier::get_value() const { return _value; }

void GamePropertyModifier::set_time_limit(float p_limit) { time_limit = p_limit; }
float GamePropertyModifier::get_time_limit() const { return time_limit; }

// Class Functions
void GamePropertyModifier::modifier_start() {
    starting_value = _value;
    emit_signal("started_modifying");
}

void GamePropertyModifier::game_process(double delta) {
    if (game_processed) return;
    
    if (type != ModifierType::TIMED && type != ModifierType::TIMED_DIMINISHING) {
        return;
    }
    
    current_time += static_cast<float>(delta);
    
    if (current_time >= time_limit) {
        if (type == ModifierType::TIMED_DIMINISHING) {
            _value = 0; // Bypass setter to avoid firing value_changed mid-process, unless intended by design
        }
        modifier_stop();
        return;
    }
    
    if (type == ModifierType::TIMED_DIMINISHING && time_limit > 0.0f) {
        float p = current_time / time_limit;
        _value = static_cast<int>(starting_value * (1.0f - p));
    }
    
    game_processed = true;
}

void GamePropertyModifier::game_process_clear() {
    game_processed = false;
}

void GamePropertyModifier::modifier_stop() {
    emit_signal("stopped_modifying");
}

godot::Dictionary GamePropertyModifier::save_data() const {
    godot::Dictionary data;
    data["type"] = type;
    data["value"] = _value;
    data["time_limit"] = time_limit;
    data["current_time"] = current_time;
    data["starting_value"] = starting_value;
    return data;
}

void GamePropertyModifier::load_data(const godot::Dictionary& data) {
    if (data.has("type")) type = static_cast<ModifierType>(static_cast<int>(data["type"]));
    if (data.has("value")) _value = data["value"];
    if (data.has("time_limit")) time_limit = data["time_limit"];
    if (data.has("current_time")) current_time = data["current_time"];
    if (data.has("starting_value")) starting_value = data["starting_value"];
}

} // namespace ideam::godot_ext