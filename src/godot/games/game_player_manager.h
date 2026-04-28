#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GamePlayer;
class GamePlayerProfile; // Assuming this extends godot::Resource

// DOD NOTE: While Singletons managing a list of pointers is a common Godot pattern, 
// a globally accessible `TypedArray` of `GamePlayer` objects creates a textbook 
// Array of Structures (AoS) cache bottleneck. Iterating over `players` forces the 
// CPU to fetch scattered heap allocations. In a strict DOD architecture, this manager 
// should internally hold dense, parallel arrays (Structure of Arrays - SoA) representing 
// player states, utilizing `std::vector` and integer handles to sidestep pointer chasing.
class GamePlayerManager : public godot::Node {
    GDCLASS(GamePlayerManager, godot::Node)

protected:
    static void _bind_methods();

private:
    static GamePlayerManager* _singleton;

    bool use_player_profiles = false;
    godot::Ref<GamePlayerProfile> default_player_profile;

    // Kept as TypedArray for the Godot binding boundary, but strictly cast internally.
    godot::TypedArray<GamePlayer> players;

public:
    GamePlayerManager();
    ~GamePlayerManager();

    virtual void _ready() override;

    // Singleton Accessor for zero-cost static lookup
    static GamePlayerManager* get_singleton();

    // Setters / Getters
    void set_use_player_profiles(bool p_use);
    bool get_use_player_profiles() const;

    void set_default_player_profile(const godot::Ref<GamePlayerProfile>& p_profile);
    godot::Ref<GamePlayerProfile> get_default_player_profile() const;

    void set_players(const godot::TypedArray<GamePlayer>& p_players);
    godot::TypedArray<GamePlayer> get_players() const;

    // Class Functions
    void load_player_profile(const godot::String& profile_path, GamePlayer* player);
    void save_player_profile(GamePlayer* player);
    
    void login_player(GamePlayer* player);
    void logout_player(GamePlayer* player);
};

} // namespace ideam::godot_ext