#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Character;
class Location;

class Prop : public Narreme {
    GDCLASS(Prop, Narreme)

private:
    // DOD NOTE: Storing direct references forces pointer-chasing and breaks 
    // spatial locality. In IDEAM Core, 'possessor' and 'current_location' 
    // will just be 32-bit or 64-bit integer IDs. We will map these using a 
    // SparseSetView for O(1) ECS-style dense lookups[cite: 33].
    godot::Ref<Character> possessor;
    godot::Ref<Location> current_location;

protected:
    static void _bind_methods();

public:
    Prop();
    ~Prop();

    virtual void initialize(); // Replaces _ready()

    // Getters / Setters
    void set_possessor(const godot::Ref<Character> &p_possessor);
    godot::Ref<Character> get_possessor() const;

    void set_current_location(const godot::Ref<Location> &p_location);
    godot::Ref<Location> get_current_location() const;

    // Class Functions
    void taken_by(const godot::Ref<Character> &new_possessor);
    void released_by(const godot::Ref<Character> &old_possessor);
    void become_lost();
    void found_by(const godot::Ref<Character> &finder);
    
    void enter_location(const godot::Ref<Location> &new_location);
    void leave_current_location();

    // Overrides
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext