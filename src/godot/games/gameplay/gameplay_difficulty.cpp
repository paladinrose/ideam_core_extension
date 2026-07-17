#include "gameplay_difficulty.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GameplayDifficulty::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulty_tags", "tags"), &GameplayDifficulty::set_difficulty_tags);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulty_tags"), &GameplayDifficulty::get_difficulty_tags);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "difficulty_tags", godot::PROPERTY_HINT_TYPE_STRING, "String"), "set_difficulty_tags", "get_difficulty_tags");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("apply_difficulty", "other"), &GameplayDifficulty::apply_difficulty);
}

GameplayDifficulty::GameplayDifficulty() {
    // DOD NOTE: Initializing arrays with specific capacities can help 
    // reduce heap churn, though GDExtension TypedArrays handle their 
    // own internal memory management.
}

GameplayDifficulty::~GameplayDifficulty() {
}

// Setters / Getters
void GameplayDifficulty::set_difficulty_tags(const godot::TypedArray<godot::String> &p_tags) {
    if (p_tags == difficulty_tags) return;
    difficulty_tags = p_tags;
    emit_changed();
}

godot::TypedArray<godot::String> GameplayDifficulty::get_difficulty_tags() const {
    return difficulty_tags;
}

// Class Functions
void GameplayDifficulty::apply_difficulty(const godot::Ref<GameplayDifficulty> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }
    // Logic for applying/merging difficulty settings would be implemented here.
}

} // namespace ideam::godot_ext