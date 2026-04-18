#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: GameplayDifficulty currently manages difficulty settings using 
 * string-based tags. In a Data-Oriented architecture, searching through 
 * Arrays of Strings is extremely inefficient due to heap indirection and 
 * linear comparison overhead. 
 *
 * For high-performance systems, these tags should be refactored into a 
 * "Difficulty Bitmask" or a contiguous array of interned 32-bit integer 
 * hashes. This allows the Difficulty System to evaluate entity compatibility 
 * using fast bitwise AND operations or SIMD integer comparisons.
 */
class GameplayDifficulty : public GameplayResource {
    GDCLASS(GameplayDifficulty, GameplayResource)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: Storing tags as TypedArray<String> forces the CPU to chase 
    // pointers to disparate heap locations for every comparison. 
    // Replacing this with a flat std::vector<uint32_t> of pre-computed 
    // hashes would significantly improve cache locality.
    godot::TypedArray<godot::String> difficulty_tags;

public:
    GameplayDifficulty();
    ~GameplayDifficulty();

    // Setters / Getters
    void set_difficulty_tags(const godot::TypedArray<godot::String> &p_tags);
    godot::TypedArray<godot::String> get_difficulty_tags() const;

    /**
     * DOD NOTE: This application logic is currently a placeholder for 
     * object-level merging. In a DOD system, this would involve bulk-copying 
     * POD data into a System's "Active Configuration Buffer" for fast access.
     */
    void apply_difficulty(const godot::Ref<GameplayDifficulty> &p_other);
};

} // namespace ideam::godot_ext