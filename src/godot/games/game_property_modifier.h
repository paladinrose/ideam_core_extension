#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// DOD NOTE: Modifier types are excellent candidates for bitwise flags or flat 
// integer IDs within a Data-Oriented ECS. Extracting them here allows a central 
// `PropertySystem` to iterate over an array of pure data structs 
// (e.g., `std::vector<ModifierData>`) rather than resolving virtual Node methods.
enum ModifierType : int32_t {
    STATIC = 0,
    TIMED = 1,
    TIMED_DIMINISHING = 2,
    RESOURCE = 3
};

// DOD NOTE: While currently inheriting from `Node` to match the GDScript API, 
// GamePropertyModifier is fundamentally just a math operation with a timer. 
// In a highly optimized C++ pipeline, this entire class should be reduced to a 
// 16-byte POD (Plain Old Data) struct: `struct Mod { int val; float time; float limit; uint8_t type; }`. 
// Emitting signals for mathematical adjustments per-frame causes severe vtable bloat and ruins L1 cache coherence.
class GamePropertyModifier : public godot::Node {
    GDCLASS(GamePropertyModifier, godot::Node)

protected:
    static void _bind_methods();

private:
    ModifierType type = ModifierType::STATIC;
    int _value = 0;
    float time_limit = 0.0f;

    float current_time = 0.0f;
    int starting_value = 0;
    bool game_processed = false;

public:
    GamePropertyModifier();
    ~GamePropertyModifier();

    // Setters / Getters
    void set_type(ModifierType p_type);
    ModifierType get_type() const;

    void set_value(int p_value);
    int get_value() const;

    void set_time_limit(float p_limit);
    float get_time_limit() const;

    // Class Functions
    void modifier_start();
    void game_process(double delta);
    void game_process_clear();
    void modifier_stop();

    // Persistence
    godot::Dictionary save_data() const;
    void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ModifierType);