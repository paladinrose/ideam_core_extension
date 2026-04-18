#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: GameplayResource serves as a base data container for gameplay-related 
 * configuration. While inheriting from godot::Resource provides easy serialization 
 * and editor integration, it enforces an Array of Structures (AoS) layout where 
 * every instance is a heap-allocated object with its own vtable.
 *
 * For systems that require high-speed access to metadata (like titles) for 
 * thousands of resources, consider a "Resource Table" approach: storing raw POD 
 * data in a contiguous buffer and using 32-bit integer IDs as handles instead 
 * of passing around heavy Resource pointers.
 */
class GameplayResource : public godot::Resource {
    GDCLASS(GameplayResource, godot::Resource)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: Storing a String object here involves heap allocation for the 
    // character data. In a performance-critical DOD loop, titles should be 
    // replaced with StringNames or interned integer hashes to allow for 
    // O(1) comparison and zero-allocation lookups.
    godot::String title;

public:
    GameplayResource();
    ~GameplayResource();

    // Setters / Getters
    void set_title(const godot::String &p_title);
    godot::String get_title() const;
};

} // namespace ideam::godot_ext