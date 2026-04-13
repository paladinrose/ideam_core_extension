#pragma once

#include "incident_condition.h"
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class Causal_Condition : public Incident_Condition {
    GDCLASS(Causal_Condition, Incident_Condition)

private:
    // DOD NOTE: Nested arrays of pointers inside polymorphic classes are a 
    // worst-case scenario for CPU caching. In IDEAM, nested causation will 
    // be mapped using a hierarchical BridgeView.
    godot::TypedArray<Incident_Condition> conditions;
    godot::TypedArray<int> causes; // Stores NarremeConditionStatus cast as ints

protected:
    static void _bind_methods();

public:
    Causal_Condition();
    ~Causal_Condition();

    void set_conditions(const godot::TypedArray<Incident_Condition> &p_conditions);
    godot::TypedArray<Incident_Condition> get_conditions() const;

    void set_causes(const godot::TypedArray<int> &p_causes);
    godot::TypedArray<int> get_causes() const;

    void add_condition(const godot::Ref<Incident_Condition> &condition, NarremeConditionStatus cause);
    void remove_condition_at(int id);

    virtual NarremeConditionStatus evaluate_condition() override;
};

} // namespace ideam::godot_ext