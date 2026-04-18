#include "game_player_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

// Project Forward Declarations
#include "game_player.h"
#include "game_player_profile.h"

namespace ideam::godot_ext {

// Initialize static singleton pointer
GamePlayerManager* GamePlayerManager::_singleton = nullptr;

void GamePlayerManager::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("player_logged_in", godot::PropertyInfo(godot::Variant::OBJECT, "player", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_logged_out", godot::PropertyInfo(godot::Variant::OBJECT, "player", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_use_player_profiles", "use"), &GamePlayerManager::set_use_player_profiles);
    godot::ClassDB::bind_method(godot::D_METHOD("get_use_player_profiles"), &GamePlayerManager::get_use_player_profiles);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "use_player_profiles"), "set_use_player_profiles", "get_use_player_profiles");

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_player_profile", "profile"), &GamePlayerManager::set_default_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_player_profile"), &GamePlayerManager::get_default_player_profile);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "default_player_profile", godot::PROPERTY_HINT_RESOURCE_TYPE, "GamePlayerProfile"), "set_default_player_profile", "get_default_player_profile");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_player_profile_safe"), &GamePlayerManager::get_default_player_profile_safe);
    godot::ClassDB::bind_method(godot::D_METHOD("load_player_profile", "profile_path", "player"), &GamePlayerManager::load_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("save_player_profile", "player"), &GamePlayerManager::save_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("login_player", "player"), &GamePlayerManager::login_player);
    godot::ClassDB::bind_method(godot::D_METHOD("logout_player", "player"), &GamePlayerManager::logout_player);
    
    godot::ClassDB::bind_static_method("GamePlayerManager", godot::D_METHOD("get_manager"), &GamePlayerManager::get_manager);
}

GamePlayerManager::GamePlayerManager() {}

GamePlayerManager::~GamePlayerManager() {
    if (_singleton == this) {
        _singleton = nullptr;
    }
}

void GamePlayerManager::_ready() {
    if (!_singleton) {
        _singleton = this;
    }
}

// Setters / Getters
void GamePlayerManager::set_use_player_profiles(bool p_use) { use_player_profiles = p_use; }
bool GamePlayerManager::get_use_player_profiles() const { return use_player_profiles; }

void GamePlayerManager::set_default_player_profile(const godot::Ref<GamePlayerProfile>& p_profile) { default_player_profile = p_profile; }
godot::Ref<GamePlayerProfile> GamePlayerManager::get_default_player_profile() const { return default_player_profile; }

void GamePlayerManager::set_players(const godot::TypedArray<GamePlayer>& p_players) { players = p_players; }
godot::TypedArray<GamePlayer> GamePlayerManager::get_players() const { return players; }

// Class Functions
godot::Ref<GamePlayerProfile> GamePlayerManager::get_default_player_profile_safe() {
    if (!default_player_profile.is_valid()) {
        // Assume GamePlayerProfile exists and can be instantiated.
        // default_player_profile.instantiate();
    }
    return default_player_profile;
}

void GamePlayerManager::load_player_profile(const godot::String& profile_path, GamePlayer* player) {
    if (!godot::ResourceLoader::get_singleton()->exists(profile_path) || !player) {
        return;
    }
    
    for (int i = 0; i < players.size(); ++i) {
        GamePlayer* p = godot::Object::cast_to<GamePlayer>(players[i]);
        if (p == player) continue;
        
        // Assuming GamePlayer exposes get_profile_path()
        // if (p && p->get_profile_path() == profile_path) return; 
    }
    
    godot::Ref<godot::Resource> loaded_res = godot::ResourceLoader::get_singleton()->load(profile_path);
    // godot::Ref<GamePlayerProfile> player_profile = loaded_res;
    
    // player->set_player_profile(player_profile);
    // player->set_profile_path(profile_path);
}

void GamePlayerManager::save_player_profile(GamePlayer* player) {
    if (!player) return;
    
    // Assuming GamePlayer API implementation:
    // godot::Ref<GamePlayerProfile> profile = player->get_player_profile();
    // godot::String path = player->get_profile_path();
    
    // if (profile == default_player_profile) return;
    // if (!profile.is_valid() || path.is_empty()) return;
    
    // godot::ResourceSaver::get_singleton()->save(profile, path);
}

void GamePlayerManager::login_player(GamePlayer* player) {
    if (!player || players.has(player)) return;
    
    players.append(player);
    emit_signal("player_logged_in", player);
}

void GamePlayerManager::logout_player(GamePlayer* player) {
    if (!player) return;
    
    int id = players.find(player);
    if (id < 0) return;
    
    // Note: Assuming `set("game_hub", nullptr)` or setter method to match GDScript `player.game_hub = null`
    player->set("game_hub", godot::Variant());
    
    // DOD NOTE: Removing an element from the middle of a `TypedArray` has an O(N) cost 
    // because it requires a memory shift. For tracking active players dynamically without 
    // order dependency, leverage the swap-and-pop technique against a contiguous raw array or `std::vector`.
    players.remove_at(id); 
    
    emit_signal("player_logged_out", player);
}

// Static Accessor
GamePlayerManager* GamePlayerManager::get_manager() {
    return _singleton;
}

} // namespace ideam::godot_ext