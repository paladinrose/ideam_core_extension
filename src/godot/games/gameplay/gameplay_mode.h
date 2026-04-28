#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameplayPerspective;
class GameplayDifficulty;
class GameplayControlMap;
class GameplayAccessibilityOption;

/**
 * DOD NOTE: GameplayMode acts as a collection of gameplay-altering components. 
 * Storing these in separate TypedArrays of Objects (AoS) creates a fragmented memory 
 * profile. In a high-performance C++26 environment, these should be refactored 
 * into a single contiguous Block of Memory (SoA) where components are stored as 
 * raw POD data, minimizing cache misses during mode initialization and updates.
 */
class GameplayMode : public GameplayResource {
    GDCLASS(GameplayMode, GameplayResource)

protected:
    static void _bind_methods();

private:
    godot::TypedArray<GameplayPerspective> perspectives;
    godot::TypedArray<GameplayDifficulty> difficulties;
    godot::TypedArray<GameplayControlMap> control_maps;
    godot::TypedArray<GameplayAccessibilityOption> accessibility_options;

public:
    GameplayMode();
    ~GameplayMode();

    // Setters / Getters
    void set_perspectives(const godot::TypedArray<GameplayPerspective> &p_perspectives);
    godot::TypedArray<GameplayPerspective> get_perspectives() const;

    void set_difficulties(const godot::TypedArray<GameplayDifficulty> &p_difficulties);
    godot::TypedArray<GameplayDifficulty> get_difficulties() const;

    void set_control_maps(const godot::TypedArray<GameplayControlMap> &p_control_maps);
    godot::TypedArray<GameplayControlMap> get_control_maps() const;

    void set_accessibility_options(const godot::TypedArray<GameplayAccessibilityOption> &p_options);
    godot::TypedArray<GameplayAccessibilityOption> get_accessibility_options() const;

    // Class Functions
    
    /**
     * DOD NOTE: This application logic utilizes nested loops and string-based comparisons 
     * for 'title' matching. This is an O(N*M) operation that stalls the CPU prefetcher. 
     * Using integer-based Component IDs or StringNames stored in contiguous memory 
     * would allow for much faster, cache-friendly lookups.
     */
    void apply_mode(const godot::Ref<GameplayMode> &p_other);
};

} // namespace ideam::godot_ext