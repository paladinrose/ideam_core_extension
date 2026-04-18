#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GamePropertyModifier;

// DOD NOTE: GameProperty encapsulates a single numeric value modified dynamically at runtime. 
// However, inheriting from `Node` binds this data to the heavy Godot scene tree. 
// In a fully optimized C++26 ECS architecture, properties should be decoupled from the Node hierarchy 
// entirely. They should be reduced to a dense SoA (Structure of Arrays) format where all active 
// `GameProperty` states (current, min, max, flags) are evaluated continuously in contiguous memory 
// arrays by a specialized System, allowing the CPU to auto-vectorize value capping and modification.
class GameProperty : public godot::Node {
    GDCLASS(GameProperty, godot::Node)

protected:
    static void _bind_methods();

private:
    godot::String title;
    
    // DOD NOTE: Packing these booleans into a single 8-bit or 16-bit bitfield (e.g., `uint16_t property_flags;`) 
    // reduces the memory footprint of this class from multiple bytes down to a fraction of a machine word, 
    // significantly improving cache line utilization when iterating through thousands of properties.
    bool restrict_value_to_current_max_value = true;
    bool restrict_value_to_current_min_value = true;
    
    bool allow_value_set = true;
    bool allow_value_modifiers = true;
    
    bool allow_max_value_set = true;
    bool allow_max_modifiers = true;
    
    bool allow_min_value_set = true;
    bool allow_min_modifiers = true;

    bool game_processed = false;
    bool is_exhausted = false;
    bool locked = false;

    int _value = 0;
    int _max_value = 10;
    int _min_value = 0;

    // DOD NOTE: Tracking modifiers as arrays of Godot Object pointers guarantees L1/L2 cache misses 
    // when calculating `get_modded_value()` during the hot path. Modifiers should be implemented 
    // as flat integer arrays or structs allocated from a `std::pmr::monotonic_buffer_resource` 
    // to keep arithmetic operations tightly bound to the CPU cache.
    godot::TypedArray<GamePropertyModifier> modifiers;
    godot::TypedArray<GamePropertyModifier> max_modifiers;
    godot::TypedArray<GamePropertyModifier> min_modifiers;

public:
    GameProperty();
    ~GameProperty();

    // Setters / Getters
    void set_title(const godot::String& p_title);
    godot::String get_title() const;

    void set_restrict_value_to_current_max_value(bool p_restrict);
    bool get_restrict_value_to_current_max_value() const;

    void set_restrict_value_to_current_min_value(bool p_restrict);
    bool get_restrict_value_to_current_min_value() const;

    void set_allow_value_set(bool p_allow);
    bool get_allow_value_set() const;

    void set_allow_value_modifiers(bool p_allow);
    bool get_allow_value_modifiers() const;

    void set_allow_max_value_set(bool p_allow);
    bool get_allow_max_value_set() const;

    void set_allow_max_modifiers(bool p_allow);
    bool get_allow_max_modifiers() const;

    void set_allow_min_value_set(bool p_allow);
    bool get_allow_min_value_set() const;

    void set_allow_min_modifiers(bool p_allow);
    bool get_allow_min_modifiers() const;

    void set_value(int cv);
    int get_value() const;

    void set_max_value(int mv);
    int get_max_value() const;

    void set_min_value(int mv);
    int get_min_value() const;

    void set_locked(bool p_locked);
    bool get_locked() const;

    void set_is_exhausted(bool p_exhausted);
    bool get_is_exhausted() const;

    // Modifiers Arrays Setters/Getters
    void set_modifiers(const godot::TypedArray<GamePropertyModifier>& p_modifiers);
    godot::TypedArray<GamePropertyModifier> get_modifiers() const;

    void set_max_modifiers(const godot::TypedArray<GamePropertyModifier>& p_modifiers);
    godot::TypedArray<GamePropertyModifier> get_max_modifiers() const;

    void set_min_modifiers(const godot::TypedArray<GamePropertyModifier>& p_modifiers);
    godot::TypedArray<GamePropertyModifier> get_min_modifiers() const;

    // Core Calculation Functions
    int get_modded_value() const;
    void send_modded_value_label();
    int get_passive_value() const;

    int get_modded_max_value() const;
    int get_modded_min_value() const;

    // Value Modifier Functions
    void add_modifier(GamePropertyModifier* mod);
    int get_modifier_id(GamePropertyModifier* modifier) const;
    void remove_modifier(GamePropertyModifier* modifier);
    void remove_modifier_at(int modID);

    // Max Modifier Functions
    void add_max_modifier(GamePropertyModifier* mod);
    int get_max_modifier_id(GamePropertyModifier* modifier) const;
    void remove_max_modifier(GamePropertyModifier* modifier);
    void remove_max_modifier_at(int modID);

    // Min Modifier Functions
    void add_min_modifier(GamePropertyModifier* mod);
    int get_min_modifier_id(GamePropertyModifier* modifier) const;
    void remove_min_modifier(GamePropertyModifier* modifier);
    void remove_min_modifier_at(int modID);

    // Game Loop Functions
    virtual void game_process(double delta);
    virtual void game_process_clear();

    // Interaction Functions
    int use_as_resource(int useValue);
    virtual void action_consequences(int score, const godot::Dictionary& consequences);

    // Persistence
    virtual godot::Dictionary save_data() const;
    virtual void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext