#include "gameplay_control_map.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GameplayControlMap::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("begun")); // [cite: 112]
    ADD_SIGNAL(godot::MethodInfo("timed_out")); // [cite: 112]
    ADD_SIGNAL(godot::MethodInfo("completed")); // [cite: 112]
    ADD_SIGNAL(godot::MethodInfo("failed")); // [cite: 112]

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_root_input", "input"), &GameplayControlMap::set_root_input);
    godot::ClassDB::bind_method(godot::D_METHOD("get_root_input"), &GameplayControlMap::get_root_input);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "root_input"), "set_root_input", "get_root_input"); // [cite: 112]

    godot::ClassDB::bind_method(godot::D_METHOD("set_priority", "priority"), &GameplayControlMap::set_priority);
    godot::ClassDB::bind_method(godot::D_METHOD("get_priority"), &GameplayControlMap::get_priority);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "priority"), "set_priority", "get_priority"); // [cite: 112]

    godot::ClassDB::bind_method(godot::D_METHOD("set_timeout", "timeout"), &GameplayControlMap::set_timeout);
    godot::ClassDB::bind_method(godot::D_METHOD("get_timeout"), &GameplayControlMap::get_timeout);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "timeout"), "set_timeout", "get_timeout"); // [cite: 112]

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("process_input", "delta"), &GameplayControlMap::process_input); // [cite: 112]
    godot::ClassDB::bind_method(godot::D_METHOD("apply_control_map", "other"), &GameplayControlMap::apply_control_map); // [cite: 112]
}

GameplayControlMap::GameplayControlMap() {
    // DOD NOTE: Zeroing out scalars here to ensure predictable memory layout 
    // and avoid uninitialized junk data in the L1 cache.
}

GameplayControlMap::~GameplayControlMap() {
}

// Setters / Getters
void GameplayControlMap::set_root_input(const godot::String &p_input) {
    root_input = p_input;
}

godot::String GameplayControlMap::get_root_input() const {
    return root_input;
}

void GameplayControlMap::set_priority(int p_priority) {
    priority = p_priority;
}

int GameplayControlMap::get_priority() const {
    return priority;
}

void GameplayControlMap::set_timeout(float p_timeout) {
    timeout = p_timeout;
}

float GameplayControlMap::get_timeout() const {
    return timeout;
}

// Class Functions
void GameplayControlMap::process_input(double p_delta) {
    // Implementation placeholder
}

void GameplayControlMap::apply_control_map(const godot::Ref<GameplayControlMap> &p_other) {
    // Implementation placeholder
}

} // namespace ideam::godot_ext