#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class SceneTransition;
class GamePlayer;
class Game;

class GameHub : public godot::Node {
    GDCLASS(GameHub, godot::Node)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    // DOD NOTE: Storing Arrays of Strings forces the CPU to chase pointers to the heap 
    // for each string's character data. In our C++26 optimizations, this should be refactored 
    // into a centralized String Interning Pool that issues contiguous 32-bit Integer Handles (Tokens).
    // Operations would then act purely on a fast std::span<uint32_t>.
    godot::TypedArray<godot::String> game_paths;
    godot::TypedArray<godot::String> game_titles;
    
    int startup_game = -1;

    SceneTransition* _game_loader = nullptr;
    SceneTransition* _default_game_loader = nullptr;

    godot::Node* hub_scene = nullptr;
    bool disable_hub_scene_on_game_load = false;

    // DOD NOTE: godot::Dictionary uses a variant-based hash map underneath, causing 
    // massive L1/L2 cache misses upon retrieval. For mapping `int` to pointers, 
    // a sparse set (SparseSetView) or a simple contiguous std::vector<Game*> 
    // using the game_id as the direct index will provide O(1) branchless lookups.
    godot::Dictionary loaded_games;
    
    godot::Node* hub_scene_parent = nullptr;
    bool hub_scene_disabled = false;
    godot::TypedArray<godot::String> loading_games;

    bool _is_ready = false;

    // Internal deferred startup
    void _on_process_frame_startup();

public:
    GameHub();
    ~GameHub();

    virtual void _ready() override;

    // Setters / Getters
    godot::Dictionary get_loaded_games() const;

    void set_game_paths(const godot::TypedArray<godot::String>& p_paths);
    godot::TypedArray<godot::String> get_game_paths() const;

    void set_game_titles(const godot::TypedArray<godot::String>& p_titles);
    godot::TypedArray<godot::String> get_game_titles() const;

    void set_startup_game(int p_startup);
    int get_startup_game() const;

    void set_game_loader(SceneTransition* p_loader);
    SceneTransition* get_game_loader() const;

    void set_hub_scene(godot::Node* p_scene);
    godot::Node* get_hub_scene() const;

    void set_disable_hub_scene_on_game_load(bool p_disable);
    bool get_disable_hub_scene_on_game_load() const;

    // Class Functions
    void connect_to_player(GamePlayer* player);
    void load_startup_game();
    void load_game(int game_id);
    void game_load_complete(godot::Node* game_node);
    void game_load_fail();
    void unload_game(int game_id);
    
    void disable_hub_scene();
    void enable_hub_scene();
    
    int add_game(const godot::String& gamePath);
    void remove_game(int game_id);

    // Internal Helpers
    void _create_default_game_loader();
    void _connect_to_game_loader();
    void _disconnect_from_game_loader();
};

} // namespace ideam::godot_ext