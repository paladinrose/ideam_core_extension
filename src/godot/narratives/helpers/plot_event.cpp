#include "plot_event.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "../narremes/incident.h" // Full blueprint needed to check incident status

namespace ideam::godot_ext {

void Plot_Event::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &Plot_Event::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &Plot_Event::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_incident", "incident"), &Plot_Event::set_incident);
    godot::ClassDB::bind_method(godot::D_METHOD("get_incident"), &Plot_Event::get_incident);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "incident", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident"), "set_incident", "get_incident");

    godot::ClassDB::bind_method(godot::D_METHOD("set_requirement", "requirement"), &Plot_Event::set_requirement);
    godot::ClassDB::bind_method(godot::D_METHOD("get_requirement"), &Plot_Event::get_requirement);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "requirement"), "set_requirement", "get_requirement");

    godot::ClassDB::bind_method(godot::D_METHOD("check_requirement"), &Plot_Event::check_requirement);
    godot::ClassDB::bind_method(godot::D_METHOD("_cannot_happen"), &Plot_Event::_cannot_happen);
    godot::ClassDB::bind_method(godot::D_METHOD("_happened"), &Plot_Event::_happened);

    ADD_SIGNAL(godot::MethodInfo("status_changed"));
    ADD_SIGNAL(godot::MethodInfo("incident_changed", godot::PropertyInfo(godot::Variant::OBJECT, "plot_event", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot_Event")));

    BIND_ENUM_CONSTANT(INCIDENT_REQ_HAS_NOT_HAPPENED);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_HAS_HAPPENED);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_CANNOT_HAPPEN);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_INSTANT_SUCCESS);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_INSTANT_FAILURE);
}

Plot_Event::Plot_Event() {}
Plot_Event::~Plot_Event() {}

void Plot_Event::initialize() {
    if (_is_ready) return;
    
    _is_ready = true;
    _connect_to_incident();
}

void Plot_Event::set_title(const godot::String &p_title) { title = p_title; }
godot::String Plot_Event::get_title() const { return title; }

void Plot_Event::set_incident(const godot::Ref<Incident> &p_incident) { incident = p_incident; }
godot::Ref<Incident> Plot_Event::get_incident() const { return incident; }

void Plot_Event::set_requirement(IncidentRequirement p_requirement) { requirement = p_requirement; }
IncidentRequirement Plot_Event::get_requirement() const { return requirement; }

bool Plot_Event::check_requirement() {
    if (!incident.is_valid()) {
        godot::UtilityFunctions::print("No Incident, no way!");
        return false;
    }
    
    IncidentStatus inc_status = incident->get_status();
    
    switch (requirement) {
        case INCIDENT_REQ_HAS_NOT_HAPPENED:
            return (inc_status == IncidentStatus::HAS_NOT_HAPPENED);
            
        case INCIDENT_REQ_HAS_HAPPENED:
            return (inc_status == IncidentStatus::HAS_HAPPENED);
            
        case INCIDENT_REQ_CANNOT_HAPPEN:
            return (inc_status == IncidentStatus::CANNOT_HAPPEN);
            
        case INCIDENT_REQ_INSTANT_SUCCESS:
            if (!_instant && inc_status == IncidentStatus::HAS_HAPPENED) {
                _instant = true;
                return true;
            }
            break;
            
        case INCIDENT_REQ_INSTANT_FAILURE:
            if (!_instant && inc_status == IncidentStatus::CANNOT_HAPPEN) {
                _instant = true;
                return true;
            }
            break;
    }
    
    return false;
}

void Plot_Event::_connect_to_incident() {
    if (!incident.is_valid()) return;
    
    if (!incident->is_connected("cannot_happen", godot::Callable(this, "_cannot_happen"))) {
        incident->connect("cannot_happen", godot::Callable(this, "_cannot_happen"));
    }
    
    if (!incident->is_connected("happened", godot::Callable(this, "_happened"))) {
        incident->connect("happened", godot::Callable(this, "_happened"));
    }
}

void Plot_Event::_disconnect_from_incident() {
    if (!incident.is_valid()) return;
    
    if (incident->is_connected("cannot_happen", godot::Callable(this, "_cannot_happen"))) {
        incident->disconnect("cannot_happen", godot::Callable(this, "_cannot_happen"));
    }
    
    if (incident->is_connected("happened", godot::Callable(this, "_happened"))) {
        incident->disconnect("happened", godot::Callable(this, "_happened"));
    }
}

void Plot_Event::_cannot_happen() {
    emit_signal("incident_changed", this);
}

void Plot_Event::_happened() {
    emit_signal("incident_changed", this);
}

} // namespace ideam::godot_ext