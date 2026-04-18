#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: GameplayControlMap is a prime candidate for refactoring into a 
 * "Command Pattern" system driven by a centralized Input Manager. 
 * * Storing input mappings as individual heap-allocated Resources (AoS) results 
 * in cache-thrashing during high-frequency input polling. In a Data-Oriented 
 * architecture, these mappings should be stored as raw 32-bit integer hashes 
 * in a contiguous buffer, allowing the Input System to linearly scan 
 * and resolve active control states in a single cache-efficient pass.
 */
class GameplayControlMap : public GameplayResource {
    GDCLASS(GameplayControlMap, GameplayResource)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: String-based input identifiers incur heavy comparison costs. 
    // Replacing 'root_input' with a StringName or a pre-hashed integer 
    // allows for branchless SIMD comparison during the input processing loop.
    godot::String root_input;
    int priority = 0;
    float timeout = 0.0f;

public:
    GameplayControlMap();
    ~GameplayControlMap();

    // Setters / Getters
    void set_root_input(const godot::String &p_input);
    godot::String get_root_input() const;

    void set_priority(int p_priority);
    int get_priority() const;

    void set_timeout(float p_timeout);
    float get_timeout() const;

    // Class Functions
    
    /**
     * DOD NOTE: Passing 'delta' into virtual function overrides for input 
     * processing is a bottleneck. High-performance C++26 systems should use 
     * an "Input Buffer Selection" where delta-time is applied to an entire 
     * contiguous array of timers (SoA) using auto-vectorization.
     */
    virtual void process_input(double p_delta);

    void apply_control_map(const godot::Ref<GameplayControlMap> &p_other);
};

} // namespace ideam::godot_ext