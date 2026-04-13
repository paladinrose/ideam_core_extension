#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Incident_Condition;
class Narrative_Condition;

enum IncidentStatus : int8_t {
    HAS_NOT_HAPPENED = 0,
    HAS_HAPPENED = 1,
    CANNOT_HAPPEN = 2
};

class Incident : public Narreme {
    GDCLASS(Incident, Narreme)

private:
    IncidentStatus status = IncidentStatus::HAS_NOT_HAPPENED;
    godot::TypedArray<Incident_Condition> conditions;

protected:
    static void _bind_methods();

public:
    Incident();
    ~Incident();

    virtual void initialize() override;

    // Getters & Setters
    void set_status(IncidentStatus p_status);
    IncidentStatus get_status() const;

    void set_conditions(const godot::TypedArray<Incident_Condition> &p_conditions);
    godot::TypedArray<Incident_Condition> get_conditions() const;

    // Class Functions
    void evaluate_incident();
    void gather_conditions();
    
    int create_narrative_condition(const godot::Ref<Narreme> &object, int condition, const godot::Ref<Narreme> &subject = nullptr);
    int add_condition(const godot::Ref<Incident_Condition> &new_condition);
    int get_condition_id(const godot::Ref<Incident_Condition> &condition) const;
    bool has_condition(const godot::Ref<Incident_Condition> &condition) const;
    
    NarremeConditionStatus evaluate_condition(int id);
    bool remove_condition(const godot::Ref<Incident_Condition> &condition);
    bool remove_condition_at(int condition_ID);

    // Helpers
    void _connect_to_condition(const godot::Ref<Incident_Condition> &condition);
    void _disconnect_from_condition(const godot::Ref<Incident_Condition> &condition);
    
    void _narcon_subject_change(const godot::Ref<Narrative_Condition> &narcon);
    void _narcon_object_change(const godot::Ref<Narrative_Condition> &narcon);
    void _narcon_condition_change(const godot::Ref<Narrative_Condition> &narcon);

    // Overrides
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::IncidentStatus);