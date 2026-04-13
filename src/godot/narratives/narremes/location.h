#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Character;
class Prop;

class Location : public Narreme {
    GDCLASS(Location, Narreme)

private:
    // DOD NOTE: Storing discrete TypedArrays for every Location instance causes 
    // scattered heap allocations. In IDEAM Core, we will manage this relationship 
    // via a BridgeView linking a Parent (Location) to a subdivided Child block 
    // (Characters/Props) , or a SparseSetView for O(1) ECS tracking[cite: 33].
    godot::TypedArray<Character> characters;
    godot::TypedArray<Prop> props;

protected:
    static void _bind_methods();

public:
    Location();
    ~Location();

    virtual void initialize();

    // Getters / Setters
    void set_characters(const godot::TypedArray<Character> &p_characters);
    godot::TypedArray<Character> get_characters() const;

    void set_props(const godot::TypedArray<Prop> &p_props);
    godot::TypedArray<Prop> get_props() const;

    // Character Management
    int character_entrance(const godot::Ref<Character> &character);
    int get_character_id(const godot::Ref<Character> &character) const;
    bool has_character(const godot::Ref<Character> &character) const;
    bool character_exit(const godot::Ref<Character> &character);
    bool character_exit_at(int character_ID);

    // Prop Management
    int prop_entrance(const godot::Ref<Prop> &prop);
    int get_prop_id(const godot::Ref<Prop> &prop) const;
    bool has_prop(const godot::Ref<Prop> &prop) const;
    bool prop_exit(const godot::Ref<Prop> &prop);
    bool prop_exit_at(int prop_ID);

    // Overrides
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext