#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "scene_transition.h"

namespace ideam::godot_ext {

// Forward Declarations
class GameBoard;

class GameplayStyle;
class GameInteraction;

// DOD NOTE: Extracting these enums from the class scope allows them to be used directly 
// in dense Structure of Arrays (SoA) containers without requiring the inclusion 
// of the heavy `Game` class definition, facilitating SIMD-friendly branching.
enum GameState : int32_t {
    UNINITIALIZED = 0,
    PREGAME = 1,
    PLAYING = 2,
    PAUSED = 3,
    RESOLUTION = 4,
    COMPLETE = 5
};

enum SaveOptions : int32_t {
    NO_SAVE = 0,
    PLAYSTATE_SAVE = 1,
    SAVE_FILES = 2,
    FULL_SAVE = 3
};

enum LoadOptions : int32_t {
    NO_LOAD = 0,
    RESUME_PLAYSTATE = 1,
    LOAD_FILES = 2,
    FULL_LOAD = 3
};

class Game : public godot::Node {
    GDCLASS(Game, godot::Node)

protected:
    static void _bind_methods();

private:
    godot::Node* game_root = nullptr;
    godot::String title = "Game Title";
    float time_scale = 1.0f;
    GameState game_state = GameState::UNINITIALIZED;
    bool close_hub_on_quit = false;

    godot::String save_path;
    SaveOptions save_options = SaveOptions::NO_SAVE;
    int max_save_files = 1;
    LoadOptions load_options = LoadOptions::NO_LOAD;

    int new_game_board = 0;
    bool unload_game_menu_scene_on_start = false;
    godot::Node* game_menu_scene = nullptr;
    bool start_game_on_load = false;

    godot::TypedArray<godot::String> game_board_paths;
    godot::TypedArray<godot::String> game_board_titles;

    SceneTransition* _game_board_loader = nullptr;
    SceneTransition* _default_board_loader = nullptr;

    int maximum_interactions = 0;
    godot::Ref<GameplayStyle> gameplay_style;

    int game_hub_ID = -1;
    
    // DOD NOTE: Storing Nodes in a Godot Dictionary mapping forces pointer chasing 
    // across disjointed heap locations. For the critical `_process` loop, this must 
    // be migrated to a `std::vector<GameBoard*>` or a flat sparse set.
    godot::Dictionary loaded_game_boards;
    
    godot::TypedArray<godot::String> loading_game_boards;
    godot::TypedArray<godot::String> loading_dependencies;
    bool dependencies_loading = false;

    godot::Ref<GameplayStyle> resolved_gameplay_style;
    bool game_is_paused = false;

    godot::TypedArray<godot::String> save_file_paths;
    godot::String continue_play_state;

    // DOD NOTE: Object pooling `Node` instances still incurs vtable lookup and memory 
    // indirection overhead. In C++26, interactions should be modeled as pure data structs 
    // in a `std::pmr::monotonic_buffer_resource` to guarantee contiguous layout and zero-fragmentation.
    godot::TypedArray<godot::Object> interactions_pool;

    bool _is_ready = false;

public:
    Game();
    ~Game();

    virtual void _ready() override;
    virtual void _process(double delta) override;

    // Setters / Getters
    void set_game_root(godot::Node* p_root);
    godot::Node* get_game_root() const;

    void set_title(const godot::String& p_title);
    godot::String get_title() const;

    void set_time_scale(float p_scale);
    float get_time_scale() const;

    void set_game_state(GameState p_state);
    GameState get_game_state() const;

    void set_close_hub_on_quit(bool p_close);
    bool get_close_hub_on_quit() const;

    void set_save_path(const godot::String& p_path);
    godot::String get_save_path() const;

    void set_save_options(SaveOptions p_options);
    SaveOptions get_save_options() const;

    void set_max_save_files(int p_max);
    int get_max_save_files() const;

    void set_load_options(LoadOptions p_options);
    LoadOptions get_load_options() const;

    void set_new_game_board(int p_board);
    int get_new_game_board() const;

    void set_unload_game_menu_scene_on_start(bool p_unload);
    bool get_unload_game_menu_scene_on_start() const;

    void set_game_menu_scene(godot::Node* p_scene);
    godot::Node* get_game_menu_scene() const;

    void set_start_game_on_load(bool p_start);
    bool get_start_game_on_load() const;

    void set_game_board_paths(const godot::TypedArray<godot::String>& p_paths);
    godot::TypedArray<godot::String> get_game_board_paths() const;

    void set_game_board_titles(const godot::TypedArray<godot::String>& p_titles);
    godot::TypedArray<godot::String> get_game_board_titles() const;

    void set_game_board_loader(SceneTransition* p_loader);
    SceneTransition* get_game_board_loader() const;

    void set_maximum_interactions(int p_max);
    int get_maximum_interactions() const;

    void set_gameplay_style(const godot::Ref<GameplayStyle>& p_style);
    godot::Ref<GameplayStyle> get_gameplay_style() const;

    // Class Methods
    void load_dependencies(const godot::TypedArray<godot::String>& dependencies);
    void game_loaded_from_game_hub();
    void start_game();
    void new_game();
    void reset_to_menu();
    void load_game(int fileID);
    void continue_in_progress();
    void save_game();
    void save_game_file(const godot::String& save_name, const godot::TypedArray<godot::String>& save_file);
    void pause_game();
    void quit_game();
    
    void load_game_board(int game_board_ID);
    void game_board_load_complete(godot::Node* game_board_node);
    void game_board_load_fail();
    void unload_game_board(int id);
    void toggle_game_board_enabled(int board_id);
    
    GameInteraction* request_game_interaction();
    void apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle);
    
    godot::Dictionary save_data() const;
    void load_data(const godot::Dictionary& data);
    void validate_continue_play_state();
    void create_game_directory();
    godot::String path_safe(const godot::String& unsafe_path) const;

    // Internal Helpers
    void _create_default_game_board_loader();
    void _connect_to_game_board_loader();
    void _disconnect_from_game_board_loader();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::GameState);
VARIANT_ENUM_CAST(ideam::godot_ext::SaveOptions);
VARIANT_ENUM_CAST(ideam::godot_ext::LoadOptions);