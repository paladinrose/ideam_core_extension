#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameProperty;

// DOD NOTE: Extracting Enums from the object scope allows for externalized state arrays. 
// A central ECS coordinator can iterate over a flat `std::span<ActionStatus>` to evaluate 
// active actions across the board without ever loading the heavy Node structures into the L1 cache.
enum ActionStatus : int32_t {
    IDLE = 0,
    STARTED = 1,
    IN_PROGRESS = 2,
    INTERRUPTED = 3,
    SUCCESSFUL = 4,
    FAILED = 5
};

class GamePieceAction : public godot::Node {
    GDCLASS(GamePieceAction, godot::Node)

protected:
    static void _bind_methods();

private:
    bool lock_piece = false;

    // DOD NOTE: Maintaining three parallel Godot Arrays (`game_properties`, `property_locks`, 
    // `property_use_values`) introduces massive heap indirection and Variant overhead. 
    // In our C++26 optimizations, this should be collapsed into a contiguous 
    // `std::vector<PropertyUsage>` struct to guarantee memory locality during the `refresh_action` loop.
    godot::TypedArray<GameProperty> game_properties;
    godot::TypedArray<bool> property_locks;
    godot::TypedArray<int> property_use_values;

    float refresh_time = 0.0f;
    godot::TypedArray<godot::String> competes_with_groups;
    ActionStatus status = ActionStatus::IDLE;

    godot::Dictionary success_consequences;
    godot::Dictionary failure_consequences;
    godot::Dictionary interruption_consequences;

    float current_time = 0.0f;
    bool refresh_properties = true;
    int value = 0;
    int current_value = 0;

    // DOD NOTE: Using a Dictionary to map String names to integer IDs for missing properties 
    // forces expensive string hashing. Replacing this with an integer-indexed bitset or a 
    // flat integer array will provide O(1) branchless lookups.
    godot::Dictionary missing_property_ids;

public:
    GamePieceAction();
    ~GamePieceAction();

    // Setters / Getters
    void set_lock_piece(bool p_lock);
    bool get_lock_piece() const;

    void set_game_properties(const godot::TypedArray<GameProperty>& p_properties);
    godot::TypedArray<GameProperty> get_game_properties() const;

    void set_property_locks(const godot::TypedArray<bool>& p_locks);
    godot::TypedArray<bool> get_property_locks() const;

    void set_property_use_values(const godot::TypedArray<int>& p_values);
    godot::TypedArray<int> get_property_use_values() const;

    void set_refresh_time(float p_time);
    float get_refresh_time() const;

    void set_competes_with_groups(const godot::TypedArray<godot::String>& p_groups);
    godot::TypedArray<godot::String> get_competes_with_groups() const;

    void set_status(ActionStatus p_status);
    ActionStatus get_status() const;

    void set_success_consequences(const godot::Dictionary& p_consequences);
    godot::Dictionary get_success_consequences() const;

    void set_failure_consequences(const godot::Dictionary& p_consequences);
    godot::Dictionary get_failure_consequences() const;

    void set_interruption_consequences(const godot::Dictionary& p_consequences);
    godot::Dictionary get_interruption_consequences() const;

    // Class Functions
    godot::TypedArray<godot::String> gather_game_property_titles() const;
    int add_property(GameProperty* property, int use_value = 0, bool lock = false);
    void exhaust_property(int local_property_id);
    int restore_property(GameProperty* property);
    bool has_property(const godot::String& property_name) const;
    GameProperty* get_property(const godot::String& property_name) const;
    int get_property_id(const godot::String& property_name) const;
    bool missing_properties() const;
    bool remove_property(GameProperty* game_property);
    bool remove_property_at(int id);

    int get_action_value();
    void start_action();
    void refresh_action();
    void update_action(double delta, int valueChange);
    void stop_action();
    void interrupt_action(GamePieceAction* interrupter);
    void end_action();
    void action_success();
    void action_failure();

    void action_consequences(int score, const godot::Dictionary& consequences);

    godot::Dictionary save_data() const;
    void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ActionStatus);