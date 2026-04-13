#include "causal_condition.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void Causal_Condition::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_conditions", "conditions"), &Causal_Condition::set_conditions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_conditions"), &Causal_Condition::get_conditions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "conditions", godot::PROPERTY_HINT_ARRAY_TYPE, "Incident_Condition"), "set_conditions", "get_conditions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_causes", "causes"), &Causal_Condition::set_causes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_causes"), &Causal_Condition::get_causes);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "causes", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_causes", "get_causes");

    godot::ClassDB::bind_method(godot::D_METHOD("add_condition", "condition", "cause"), &Causal_Condition::add_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_condition_at", "id"), &Causal_Condition::remove_condition_at);

    ADD_SIGNAL(godot::MethodInfo("condition_added", godot::PropertyInfo(godot::Variant::OBJECT, "condition", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident_Condition"), godot::PropertyInfo(godot::Variant::INT, "cause")));
    ADD_SIGNAL(godot::MethodInfo("condition_removed", godot::PropertyInfo(godot::Variant::INT, "at")));
}

Causal_Condition::Causal_Condition() {}
Causal_Condition::~Causal_Condition() {}

void Causal_Condition::set_conditions(const godot::TypedArray<Incident_Condition> &p_conditions) { conditions = p_conditions; }
godot::TypedArray<Incident_Condition> Causal_Condition::get_conditions() const { return conditions; }

void Causal_Condition::set_causes(const godot::TypedArray<int> &p_causes) { causes = p_causes; }
godot::TypedArray<int> Causal_Condition::get_causes() const { return causes; }

void Causal_Condition::add_condition(const godot::Ref<Incident_Condition> &condition, NarremeConditionStatus cause) {
    if (conditions.has(condition) || condition.ptr() == this) return;
    
    conditions.append(condition);
    causes.append((int)cause);
    
    emit_signal("condition_added", condition, (int)cause);
}

void Causal_Condition::remove_condition_at(int id) {
    if (id < 0 || id >= conditions.size()) return;
    
    conditions.remove_at(id);
    causes.remove_at(id);
    
    emit_signal("condition_removed", id);
}

NarremeConditionStatus Causal_Condition::evaluate_condition() {
    NarremeConditionStatus new_status = NarremeConditionStatus::MET;
    
    for (int i = 0; i < conditions.size(); i++) {
        godot::Ref<Incident_Condition> condition = conditions[i];
        if (condition.ptr() == this) continue;
        
        NarremeConditionStatus condition_status = condition->get_last_status();
        if ((int)condition_status != (int)causes[i]) {
            new_status = NarremeConditionStatus::NOT_MET;
        }
    }
    
    NarremeConditionStatus current_last_status = get_last_status();
    
    switch (new_status) {
        case NarremeConditionStatus::MET:
            if (current_last_status == NarremeConditionStatus::NOT_MET) emit_signal("met");
            break;
        case NarremeConditionStatus::NOT_MET:
            if (current_last_status == NarremeConditionStatus::MET) emit_signal("unmet");
            break;
        case NarremeConditionStatus::CANNOT_MEET:
            if (current_last_status == NarremeConditionStatus::NOT_MET || current_last_status == NarremeConditionStatus::MET) {
                emit_signal("cannot_meet");
            }
            break;
        default:
            break;
    }
    
    set_last_status(new_status);
    return new_status;
}

} // namespace ideam::godot_ext