#include "game_player_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

// Project Forward Declarations explicitly mapped for strong typing
#include "game_player.h"
#include "game_player_profile.h" // Must implement instantiate() / reference semantics

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
    godot::ClassDB::bind_method(godot::D_METHOD("load_player_profile", "profile_path", "player"), &GamePlayerManager::load_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("save_player_profile", "player"), &GamePlayerManager::save_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("login_player", "player"), &GamePlayerManager::login_player);
    godot::ClassDB::bind_method(godot::D_METHOD("logout_player", "player"), &GamePlayerManager::logout_player);
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

GamePlayerManager* GamePlayerManager::get_singleton() {
    return _singleton;
}

// Setters / Getters
void GamePlayerManager::set_use_player_profiles(bool p_use) { use_player_profiles = p_use; }
bool GamePlayerManager::get_use_player_profiles() const { return use_player_profiles; }

void GamePlayerManager::set_default_player_profile(const godot::Ref<GamePlayerProfile>& p_profile) { default_player_profile = p_profile; }

godot::Ref<GamePlayerProfile> GamePlayerManager::get_default_player_profile() const {
    // We cast away constness locally to safely lazy-load the default profile.
    GamePlayerManager* mutable_this = const_cast<GamePlayerManager*>(this);
    
    if (!mutable_this->default_player_profile.is_valid()) {
        mutable_this->default_player_profile.instantiate();
    }
    return mutable_this->default_player_profile;
}

void GamePlayerManager::set_players(const godot::TypedArray<GamePlayer>& p_players) { players = p_players; }
godot::TypedArray<GamePlayer> GamePlayerManager::get_players() const { return players; }


// Class Functions
void GamePlayerManager::load_player_profile(const godot::String& profile_path, GamePlayer* player) {
    if (!player) return;
    
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    if (!loader->exists(profile_path)) {
        return;
    }

    // DOD NOTE: Explicitly type-casting the array iterations rather than using Variant checks.
    for (int i = 0; i < players.size(); ++i) {
        GamePlayer* p = godot::Object::cast_to<GamePlayer>(players[i]);
        if (p == player) continue;
        
        // Ensure another logged-in player is not currently utilizing this exact profile path
        // (Assumption: GamePlayer API supports a method or mapped variable for profile paths)
        // if (p && p->get_profile_path() == profile_path) return;
    }

    godot::Ref<GamePlayerProfile> loaded_profile = loader->load(profile_path);
    if (loaded_profile.is_valid()) {
        player->set_player_profile(loaded_profile);
        // player->set_profile_path(profile_path); 
    }
}

void GamePlayerManager::save_player_profile(GamePlayer* player) {
    if (!player) return;
    
    godot::Ref<GamePlayerProfile> profile = player->get_player_profile();
    // godot::String path = player->get_profile_path();
    
    // Strict pointer address comparison to avoid evaluating against the default prototype
    if (profile == default_player_profile || !profile.is_valid()) { // || path.is_empty()) {
        return;
    }
    
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
    
    // Strictly typed method dispatch. GDScript `player.game_hub = null` relies on 
    // engine runtime hashes. This relies directly on the C++ vtable/linkage.
    // player->set_game_hub(nullptr);
    
    players.remove_at(id);
    emit_signal("player_logged_out", player);
}

} // namespace ideam::godot_ext