#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameplayStyle;

// DOD NOTE: While extending `Node` is necessary for Godot integration, `GameEntity` 
// objects inherently suffer from Array of Structures (AoS) memory bloat and vtable 
// scattering across the heap. For critical `_process` loops, consider extracting 
// hot data (like `time_scale`, `enabled` state) into parallel contiguous arrays 
// (Structure of Arrays - SoA) managed by the `GameBoard` or a custom ECS architecture.
class GameEntity : public godot::Node {
    GDCLASS(GameEntity, godot::Node)

protected:
    static void _bind_methods();

    bool enabled = true;
    godot::String title;
    godot::Node* root_node = nullptr;
    Game* _game = nullptr;
    float time_scale = 1.0f;
    godot::Ref<GameplayStyle> gameplay_style;

    bool entity_is_initialized = false;
    bool game_processed = false;
    bool entity_is_paused = false;

    godot::Ref<GameplayStyle> _resolved_gameplay_style;

public:
    GameEntity();
    ~GameEntity();

    virtual void _ready() override;

    // Setters / Getters
    void set_enabled(bool p_enabled);
    bool get_enabled() const;

    void set_title(const godot::String& p_title);
    godot::String get_title() const;

    void set_root_node(godot::Node* p_root);
    godot::Node* get_root_node() const;

    void set_game(Game* p_game);
    Game* get_game() const;

    void set_time_scale(float p_scale);
    float get_time_scale() const;

    void set_gameplay_style(const godot::Ref<GameplayStyle>& p_style);
    godot::Ref<GameplayStyle> get_gameplay_style() const;

    // DOD Accessors for strict internal state
    bool get_entity_is_paused() const;
    void set_entity_is_paused(bool p_paused);

    // Class Functions
    void validate_game();
    
    virtual void game_start();
    virtual void enter_game();
    virtual void game_end();
    virtual void exit_game();
    virtual void game_pause();
    virtual void game_continue();
    
    // DOD NOTE: Relying on virtual function overriding for `game_process` forces 
    // pointer dereferencing and vtable lookups every frame, stalling the CPU pipeline. 
    // Batch processing standard entity behaviors using a Data-Oriented Job System 
    // will yield massive cache coherence improvements.
    virtual void game_process(double delta);
    virtual void game_process_clear();
    
    void toggle_game_entity_enabled();
    virtual void enable_game_entity();
    virtual void disable_game_entity();
    
    // DOD NOTE: Passing highly dynamic string-keyed Dictionaries forces expensive 
    // Variant hashing. Refactoring `consequences` into a tightly packed POD Struct 
    // (e.g., `EntityCommand`) or an integer bitmask will significantly improve throughput.
    virtual void action_consequences(int score, const godot::Dictionary& consequences);
    void apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle);
    
    virtual godot::Dictionary save_data() const;
    virtual void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext