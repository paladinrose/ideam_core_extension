#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

// Forward declaring a hypothetical class provided by the plugin architecture
namespace ideam::godot_ext {
class GameplayMode;

/**
 * DOD NOTE: GameplayStyle acts as a container for various gameplay modes.
 * Storing these as an Array of Object pointers (AoS) results in fragmented 
 * memory lookups when the engine needs to query style settings across 
 * thousands of entities. 
 *
 * In a pure Data-Oriented system, these modes should be refactored into 
 * "Component Bitmasks" or flat buffers of POD (Plain Old Data) structs 
 * to allow the CPU to linearly scan settings without pointer chasing.
 */
class GameplayStyle : public GameplayResource {
    GDCLASS(GameplayStyle, GameplayResource)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: Using TypedArray<GameplayMode> forces the CPU to dereference
    // pointers to disparate heap locations. For performance-critical mode 
    // switching, consider a contiguous std::vector or a custom MemoryPool.
    godot::TypedArray<GameplayMode> modes;

public:
    GameplayStyle();
    ~GameplayStyle();

    // Setters / Getters
    void set_modes(const godot::TypedArray<GameplayMode> &p_modes);
    godot::TypedArray<GameplayMode> get_modes() const;

    // Class Functions
    
    /**
     * DOD NOTE: This merge operation uses string-based title comparisons, 
     * which is O(N^2) and cache-hostile. Refactoring titles to 32-bit 
     * integer hashes or StringNames would allow for O(1) or O(log N) 
     * comparisons using SIMD-friendly integer registers.
     */
    void apply_style(const godot::Ref<GameplayStyle> &p_style);
    
    godot::Dictionary save_style() const;
    void load_style(const godot::Dictionary &p_data);
};

} // namespace ideam::godot_ext