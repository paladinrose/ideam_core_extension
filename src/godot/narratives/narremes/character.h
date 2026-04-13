#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations for other Narreme types
class Location;
class Prop;
class Relationship;
class Plot;
class Incident;
class Narrative;

class Character : public Narreme {
    GDCLASS(Character, Narreme)

private:
    // DOD NOTE: Storing these TypedArrays inside the Character object forces 
    // Array of Structures (AoS) memory layout and heap fragmentation. 
    // In IDEAM Core, these will be replaced by `BridgeView` lookups 
    // linking a `Character ID` directly to a contiguous block of `Prop IDs` or `Plot IDs`.
    godot::Ref<Location> current_location;
    godot::TypedArray<Prop> possessions;
    godot::TypedArray<Relationship> relationships;
    godot::TypedArray<Plot> goals;

protected:
    static void _bind_methods();

public:
    Character();
    ~Character();

    virtual void initialize(); // Replaces _ready()

    // Location Management
    void enter_location(const godot::Ref<Location> &p_new_location);
    void leave_current_location();
    
    godot::Ref<Location> get_current_location() const;
    void set_current_location(const godot::Ref<Location> &p_location);

    // Possession Management
    int take_possession(const godot::Ref<Prop> &p_possession);
    int get_possession_id(const godot::Ref<Prop> &p_possession) const;
    bool has_possession_of(const godot::Ref<Prop> &p_possession) const;
    void lose_possession(const godot::Ref<Prop> &p_possession);
    void lose_possession_at(int p_id);

    godot::TypedArray<Prop> get_possessions() const;
    void set_possessions(const godot::TypedArray<Prop> &p_possessions);

    // Goal Management
    int set_goal(const godot::Ref<Plot> &p_goal);
    int get_goal_id(const godot::Ref<Plot> &p_goal) const;
    bool has_goal(const godot::Ref<Plot> &p_goal) const;
    void remove_goal(const godot::Ref<Plot> &p_goal);
    void remove_goal_at(int p_goal_id);
    
    // DOD NOTE: Assuming 'PlotStatus' is an integer or enum returned by the Plot class.
    int check_goal_status(const godot::Ref<Plot> &p_goal);
    int check_goal_status_at(int p_goal_id);
    void goal_success(int p_goal_id);
    void goal_fail(int p_goal_id);

    godot::TypedArray<Plot> get_goals() const;
    void set_goals(const godot::TypedArray<Plot> &p_goals);

    // Relationship Management
    int begin_relationship(const godot::Ref<Narreme> &p_relation, const godot::String &p_title);
    bool change_relation(int p_id, const godot::Ref<Narreme> &p_new_relation);
    bool change_relationship(int p_id, const godot::String &p_new_title);
    int get_relationship_id(const godot::String &p_title) const;
    godot::TypedArray<int> get_relation_ids(const godot::Ref<Narreme> &p_target) const;

    godot::TypedArray<Relationship> get_relationships() const;
    void set_relationships(const godot::TypedArray<Relationship> &p_relationships);

    // Overrides
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext