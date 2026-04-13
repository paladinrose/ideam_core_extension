#include "incident.h"
#include <godot_cpp/core/class_db.hpp>
#include "../helpers/incident_condition.h"
#include "../helpers/narrative_condition.h"

namespace ideam::godot_ext {

void Incident::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_status", "status"), &Incident::set_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_status"), &Incident::get_status);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "status"), "set_status", "get_status");

    godot::ClassDB::bind_method(godot::D_METHOD("set_conditions", "conditions"), &Incident::set_conditions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_conditions"), &Incident::get_conditions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "conditions", godot::PROPERTY_HINT_ARRAY_TYPE, "Incident_Condition"), "set_conditions", "get_conditions");

    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_incident"), &Incident::evaluate_incident);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_conditions"), &Incident::gather_conditions);
    godot::ClassDB::bind_method(godot::D_METHOD("create_narrative_condition", "object", "condition", "subject"), &Incident::create_narrative_condition, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("add_condition", "new_condition"), &Incident::add_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("get_condition_id", "condition"), &Incident::get_condition_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_condition", "condition"), &Incident::has_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_condition", "id"), &Incident::evaluate_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_condition", "condition"), &Incident::remove_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_condition_at", "condition_ID"), &Incident::remove_condition_at);

    godot::ClassDB::bind_method(godot::D_METHOD("_narcon_subject_change", "narcon"), &Incident::_narcon_subject_change);
    godot::ClassDB::bind_method(godot::D_METHOD("_narcon_object_change", "narcon"), &Incident::_narcon_object_change);
    godot::ClassDB::bind_method(godot::D_METHOD("_narcon_condition_change", "narcon"), &Incident::_narcon_condition_change);

    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Incident::get_class_name_str);

    ADD_SIGNAL(godot::MethodInfo("cannot_happen"));
    ADD_SIGNAL(godot::MethodInfo("happened"));
    ADD_SIGNAL(godot::MethodInfo("condition_added", godot::PropertyInfo(godot::Variant::OBJECT, "new_condition", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident_Condition")));
    ADD_SIGNAL(godot::MethodInfo("condition_removed", godot::PropertyInfo(godot::Variant::OBJECT, "removed_condition", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident_Condition")));

    BIND_ENUM_CONSTANT(IncidentStatus::HAS_NOT_HAPPENED);
    BIND_ENUM_CONSTANT(IncidentStatus::HAS_HAPPENED);
    BIND_ENUM_CONSTANT(IncidentStatus::CANNOT_HAPPEN);
}

Incident::Incident() {}
Incident::~Incident() {}

void Incident::initialize() {
    Narreme::initialize();
    gather_conditions();
}

void Incident::set_status(IncidentStatus p_status) { status = p_status; }
IncidentStatus Incident::get_status() const { return status; }

void Incident::set_conditions(const godot::TypedArray<Incident_Condition> &p_conditions) { conditions = p_conditions; }
godot::TypedArray<Incident_Condition> Incident::get_conditions() const { return conditions; }

void Incident::evaluate_incident() {
    if (status == IncidentStatus::HAS_HAPPENED || status == IncidentStatus::CANNOT_HAPPEN) return;
    
    IncidentStatus new_status = IncidentStatus::HAS_HAPPENED;
    
    for (int i = 0; i < conditions.size(); i++) {
        godot::Ref<Incident_Condition> condition = conditions[i];
        if (!condition.is_valid()) continue;
        
        if (condition->get_last_status() == NarremeConditionStatus::UNKNOWN) {
            condition->evaluate_condition();
        }
        
        if (condition->get_last_status() == NarremeConditionStatus::CANNOT_MEET) {
            status = IncidentStatus::CANNOT_HAPPEN;
            emit_signal("cannot_happen");
            return;
        }
        
        if (condition->get_last_status() == NarremeConditionStatus::NOT_MET) {
            new_status = IncidentStatus::HAS_NOT_HAPPENED;
        }
    }
    
    if (new_status == IncidentStatus::HAS_HAPPENED) {
        status = IncidentStatus::HAS_HAPPENED;
        emit_signal("happened");
    }
}

void Incident::gather_conditions() {
    // DOD NOTE: Since this is now a RefCounted resource array instead of a SceneTree node, 
    // we don't clear and repopulate from children. We just ensure existing resources 
    // are properly connected.
    for (int i = 0; i < conditions.size(); i++) {
        godot::Ref<Incident_Condition> condition = conditions[i];
        if (condition.is_valid()) {
            _connect_to_condition(condition);
        }
    }
}

int Incident::create_narrative_condition(const godot::Ref<Narreme> &object, int condition, const godot::Ref<Narreme> &subject) {
    // DOD NOTE: Instantiating RefCounted memory here fragments the heap. 
    // In IDEAM Core, adding a condition will just mean writing parallel rows 
    // to the Sequence Metadata tables.
    godot::Ref<Narrative_Condition> newCondition;
    newCondition.instantiate();
    
    newCondition->set_object(object);
    newCondition->set_condition(condition);
    newCondition->set_subject(subject);
    
    int id = get_condition_id(newCondition);
    
    if (id < 0) {
        id = conditions.size();
        conditions.append(newCondition);
        _connect_to_condition(newCondition);
        emit_signal("condition_added", newCondition);
    } 
    // No need for queue_free() in else block. RefCounted drops it automatically.
    
    return id;
}

int Incident::add_condition(const godot::Ref<Incident_Condition> &new_condition) {
    int id = get_condition_id(new_condition);
    if (id < 0) {
        id = conditions.size();
        conditions.append(new_condition);
        _connect_to_condition(new_condition);
        emit_signal("condition_added", new_condition);
    }
    return id;
}

int Incident::get_condition_id(const godot::Ref<Incident_Condition> &condition) const {
    return conditions.find(condition);
}

bool Incident::has_condition(const godot::Ref<Incident_Condition> &condition) const {
    return conditions.has(condition);
}

NarremeConditionStatus Incident::evaluate_condition(int id) {
    if (id < 0 || id >= conditions.size()) return NarremeConditionStatus::CANNOT_MEET;
    
    godot::Ref<Incident_Condition> condition = conditions[id];
    if (condition.is_valid()) {
        return condition->evaluate_condition();
    }
    return NarremeConditionStatus::CANNOT_MEET;
}

bool Incident::remove_condition(const godot::Ref<Incident_Condition> &condition) {
    int condition_ID = get_condition_id(condition);
    if (condition_ID >= 0) return remove_condition_at(condition_ID);
    return false;
}

bool Incident::remove_condition_at(int condition_ID) {
    if (condition_ID < 0 || condition_ID >= conditions.size()) return false;
    
    godot::Ref<Incident_Condition> condition = conditions[condition_ID];
    _disconnect_from_condition(condition);
    conditions.remove_at(condition_ID);
    
    emit_signal("condition_removed", condition);
    return true;
}

void Incident::_connect_to_condition(const godot::Ref<Incident_Condition> &condition) {
    godot::Ref<Narrative_Condition> narcon = condition;
    if (narcon.is_valid()) {
        if (!narcon->is_connected("subject_changed", godot::Callable(this, "_narcon_subject_change"))) {
            narcon->connect("subject_changed", godot::Callable(this, "_narcon_subject_change"));
        }
        if (!narcon->is_connected("object_changed", godot::Callable(this, "_narcon_object_change"))) {
            narcon->connect("object_changed", godot::Callable(this, "_narcon_object_change"));
        }
        if (!narcon->is_connected("condition_changed", godot::Callable(this, "_narcon_condition_change"))) {
            narcon->connect("condition_changed", godot::Callable(this, "_narcon_condition_change"));
        }
    }
}

void Incident::_disconnect_from_condition(const godot::Ref<Incident_Condition> &condition) {
    godot::Ref<Narrative_Condition> narcon = condition;
    if (narcon.is_valid()) {
        if (narcon->is_connected("subject_changed", godot::Callable(this, "_narcon_subject_change"))) {
            narcon->disconnect("subject_changed", godot::Callable(this, "_narcon_subject_change"));
        }
        if (narcon->is_connected("object_changed", godot::Callable(this, "_narcon_object_change"))) {
            narcon->disconnect("object_changed", godot::Callable(this, "_narcon_object_change"));
        }
        if (narcon->is_connected("condition_changed", godot::Callable(this, "_narcon_condition_change"))) {
            narcon->disconnect("condition_changed", godot::Callable(this, "_narcon_condition_change"));
        }
    }
}

void Incident::_narcon_subject_change(const godot::Ref<Narrative_Condition> &narcon) {
    int id = get_condition_id(narcon);
}

void Incident::_narcon_object_change(const godot::Ref<Narrative_Condition> &narcon) {
    int id = get_condition_id(narcon);
}

void Incident::_narcon_condition_change(const godot::Ref<Narrative_Condition> &narcon) {
    int id = get_condition_id(narcon);
}

godot::Array Incident::get_narrative_conditions(Narreme *p_narreme) const {
    godot::Array conditions_arr;
    if (p_narreme) {
        conditions_arr.append("has condition subject");
        conditions_arr.append("has condition object");
    } else {
        conditions_arr.append("has not happened");
        conditions_arr.append("has happened");
        conditions_arr.append("cannot happen");
    }
    return conditions_arr;
}

NarremeConditionStatus Incident::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        switch (p_condition_id) {
            case 0:
                for (int i = 0; i < conditions.size(); i++) {
                    godot::Ref<Narrative_Condition> narcon = conditions[i];
                    if (narcon.is_valid() && narcon->get_subject().ptr() == p_conditional_narreme) {
                        return NarremeConditionStatus::MET;
                    }
                }
                return NarremeConditionStatus::NOT_MET;
            case 1:
                for (int i = 0; i < conditions.size(); i++) {
                    godot::Ref<Narrative_Condition> narcon = conditions[i];
                    if (narcon.is_valid() && narcon->get_object().ptr() == p_conditional_narreme) {
                        return NarremeConditionStatus::MET;
                    }
                }
                return NarremeConditionStatus::NOT_MET;
        }
    } else {
        switch (p_condition_id) {
            case 0: return (status == IncidentStatus::HAS_NOT_HAPPENED) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1: return (status == IncidentStatus::HAS_HAPPENED) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2: return (status == IncidentStatus::CANNOT_HAPPEN) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Incident::get_class_name_str() const {
    return "Incident";
}

} // namespace ideam::godot_ext