#include "game_player_profile.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GamePlayerProfile::_bind_methods() {
    // Note: Future bindings for permissions, privileges, and accomplishments 
    // will be registered here as properties or methods.
}

GamePlayerProfile::GamePlayerProfile() {
    // Initialization of bitmasks and stat arrays deferred to here
}

GamePlayerProfile::~GamePlayerProfile() {}

} // namespace ideam::godot_ext