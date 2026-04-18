#pragma once

#include <godot_cpp/classes/resource.hpp>

namespace ideam::godot_ext {

// DOD NOTE: As this profile expands to hold permissions, privileges, and achievements, 
// avoid modeling these discrete states as individual objects, nested Godot Dictionaries, 
// or sprawling Resources. Instead, utilize dense bitmasks (e.g., `uint64_t unlock_flags`) 
// for boolean states and flat, contiguous arrays (`std::vector<uint32_t>`) for numeric statistics. 
// This guarantees ultra-fast, cache-friendly serialization and O(1) bitwise permission 
// evaluations, which is critical if validating privileges across many players simultaneously.
class GamePlayerProfile : public godot::Resource {
    GDCLASS(GamePlayerProfile, godot::Resource)

protected:
    static void _bind_methods();

public:
    GamePlayerProfile();
    ~GamePlayerProfile();
};

} // namespace ideam::godot_ext