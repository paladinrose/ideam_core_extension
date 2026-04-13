#include "incident_condition.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void Incident_Condition::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &Incident_Condition::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &Incident_Condition::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_last_status", "last_status"), &Incident_Condition::set_last_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_status"), &Incident_Condition::get_last_status);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "last_status"), "set_last_status", "get_last_status");

    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_condition"), &Incident_Condition::evaluate_condition);

    ADD_SIGNAL(godot::MethodInfo("met"));
    ADD_SIGNAL(godot::MethodInfo("unmet"));
    ADD_SIGNAL(godot::MethodInfo("cannot_meet"));
    ADD_SIGNAL(godot::MethodInfo("became_unknown"));
}

Incident_Condition::Incident_Condition() {}
Incident_Condition::~Incident_Condition() {}

void Incident_Condition::initialize() {
    if (_is_ready) return;
    _is_ready = true;
    
    // Note: get_class() might be more appropriate here since RefCounted objects don't have a 'name' like Nodes do.
    if (title.is_empty()) {
        title = get_class(); 
    }
}

void Incident_Condition::set_title(const godot::String &p_title) { title = p_title; }
godot::String Incident_Condition::get_title() const { return title; }

void Incident_Condition::set_last_status(NarremeConditionStatus p_status) { last_status = p_status; }
NarremeConditionStatus Incident_Condition::get_last_status() const { return last_status; }

NarremeConditionStatus Incident_Condition::evaluate_condition() {
    return NarremeConditionStatus::UNKNOWN;
}

} // namespace ideam::godot_ext