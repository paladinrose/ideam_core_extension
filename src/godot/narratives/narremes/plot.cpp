#include "plot.h"
#include <godot_cpp/core/class_db.hpp>
#include "../helpers/plot_event.h"
#include "incident.h"

namespace ideam::godot_ext {

void Plot::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_status", "status"), &Plot::set_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_status"), &Plot::get_status);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "status"), "set_status", "get_status");

    godot::ClassDB::bind_method(godot::D_METHOD("set_plot_events", "plot_events"), &Plot::set_plot_events);
    godot::ClassDB::bind_method(godot::D_METHOD("get_plot_events"), &Plot::get_plot_events);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "plot_events", godot::PROPERTY_HINT_ARRAY_TYPE, "Plot_Event"), "set_plot_events", "get_plot_events");

    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_plot_status"), &Plot::evaluate_plot_status);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_plot_events"), &Plot::gather_plot_events);
    godot::ClassDB::bind_method(godot::D_METHOD("add_plot_event", "new_plot_event"), &Plot::add_plot_event);
    godot::ClassDB::bind_method(godot::D_METHOD("get_plot_event_id", "plot_event"), &Plot::get_plot_event_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_plot_event", "plot_event"), &Plot::has_plot_event);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_plot_event", "plot_event"), &Plot::remove_plot_event);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_plot_event_at", "plot_event_ID"), &Plot::remove_plot_event_at);

    godot::ClassDB::bind_method(godot::D_METHOD("_plot_event_change", "plot_event"), &Plot::_plot_event_change);
    
    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Plot::get_class_name_str);

    ADD_SIGNAL(godot::MethodInfo("begun"));
    ADD_SIGNAL(godot::MethodInfo("completed"));
    ADD_SIGNAL(godot::MethodInfo("cannot_complete"));
    ADD_SIGNAL(godot::MethodInfo("failed"));
    ADD_SIGNAL(godot::MethodInfo("plot_event_added"));
    ADD_SIGNAL(godot::MethodInfo("plot_event_removed"));

    BIND_ENUM_CONSTANT(PLOT_NOT_BEGUN);
    BIND_ENUM_CONSTANT(PLOT_IN_PROGRESS);
    BIND_ENUM_CONSTANT(PLOT_COMPLETE);
    BIND_ENUM_CONSTANT(PLOT_CANNOT_COMPLETE);
    BIND_ENUM_CONSTANT(PLOT_FAILED);
}

Plot::Plot() {}
Plot::~Plot() {}

void Plot::initialize() {
    Narreme::initialize();
    gather_plot_events();
}

void Plot::set_status(PlotStatus p_status) { status = p_status; }
PlotStatus Plot::get_status() const { return status; }

void Plot::set_plot_events(const godot::TypedArray<Plot_Event> &p_events) { plot_events = p_events; }
godot::TypedArray<Plot_Event> Plot::get_plot_events() const { return plot_events; }

PlotStatus Plot::evaluate_plot_status() {
    if (status == PLOT_COMPLETE || status == PLOT_CANNOT_COMPLETE || status == PLOT_FAILED) {
        return status;
    }
    
    PlotStatus s = status;
    bool allRequired = true;
    bool shouldBreak = false;
    
    for (int i = 0; i < plot_events.size(); i++) {
        godot::Ref<Plot_Event> plot_event = plot_events[i];
        if (!plot_event.is_valid()) continue;
        
        shouldBreak = false;
        
        // Grab the incident for deep checks
        godot::Ref<Incident> incident = plot_event->get_incident();
        
        switch (plot_event->get_requirement()) {
            case INCIDENT_REQ_INSTANT_FAILURE:
                if (plot_event->check_requirement()) {
                    s = PLOT_FAILED;
                    emit_signal("failed");
                    shouldBreak = true;
                }
                break;
                
            case INCIDENT_REQ_INSTANT_SUCCESS:
                if (plot_event->check_requirement()) {
                    s = PLOT_COMPLETE;
                    emit_signal("completed");
                    shouldBreak = true;
                }
                break;
                
            case INCIDENT_REQ_HAS_HAPPENED:
                if (plot_event->check_requirement()) {
                    if (s != PLOT_CANNOT_COMPLETE) {
                        s = PLOT_IN_PROGRESS;
                    }
                } else if (incident.is_valid() && incident->get_status() == IncidentStatus::CANNOT_HAPPEN) {
                    s = PLOT_CANNOT_COMPLETE;
                    emit_signal("cannot_complete");
                    allRequired = false;
                } else {
                    allRequired = false;
                }
                break;
                
            case INCIDENT_REQ_CANNOT_HAPPEN:
                if (plot_event->check_requirement()) {
                    if (s != PLOT_CANNOT_COMPLETE) {
                        s = PLOT_IN_PROGRESS;
                    }
                } else if (incident.is_valid() && incident->get_status() == IncidentStatus::HAS_HAPPENED) {
                    s = PLOT_CANNOT_COMPLETE;
                    emit_signal("cannot_complete");
                    allRequired = false;
                } else {
                    allRequired = false;
                }
                break;
                
            case INCIDENT_REQ_HAS_NOT_HAPPENED:
                // Implicit pass in GDScript version, unhandled in the original match statement
                break;
        }
        
        if (shouldBreak) break;
    }
    
    if (s == PLOT_IN_PROGRESS) {
        if (status == PLOT_NOT_BEGUN) {
            status = PLOT_IN_PROGRESS;
            emit_signal("begun");
        }
        
        if (!shouldBreak && allRequired) {
            status = PLOT_COMPLETE;
            emit_signal("completed");
            return status;
        }
    }
    
    status = s;
    return status;
}

void Plot::gather_plot_events() {
    for (int i = 0; i < plot_events.size(); i++) {
        godot::Ref<Plot_Event> plot_event = plot_events[i];
        if (plot_event.is_valid()) {
            _connect_to_plot_event(plot_event);
        }
    }
}

int Plot::add_plot_event(const godot::Ref<Plot_Event> &new_plot_event) {
    int id = get_plot_event_id(new_plot_event);
    
    if (id < 0) {
        id = plot_events.size();
        plot_events.append(new_plot_event);
        _connect_to_plot_event(new_plot_event);
        emit_signal("plot_event_added");
    }
    
    return id;
}

int Plot::get_plot_event_id(const godot::Ref<Plot_Event> &plot_event) const {
    return plot_events.find(plot_event);
}

bool Plot::has_plot_event(const godot::Ref<Plot_Event> &plot_event) const {
    return plot_events.has(plot_event);
}

bool Plot::remove_plot_event(const godot::Ref<Plot_Event> &plot_event) {
    int plot_event_ID = get_plot_event_id(plot_event);
    return remove_plot_event_at(plot_event_ID);
}

bool Plot::remove_plot_event_at(int plot_event_ID) {
    if (plot_event_ID < 0 || plot_event_ID >= plot_events.size()) {
        return false;
    }
    
    godot::Ref<Plot_Event> plot_event = plot_events[plot_event_ID];
    _disconnect_from_plot_event(plot_event);
    plot_events.remove_at(plot_event_ID);
    emit_signal("plot_event_removed");
    
    return true;
}

void Plot::_connect_to_plot_event(const godot::Ref<Plot_Event> &plot_event) {
    if (!plot_event.is_valid()) return;
    if (!plot_event->is_connected("incident_changed", godot::Callable(this, "_plot_event_change"))) {
        plot_event->connect("incident_changed", godot::Callable(this, "_plot_event_change"));
    }
}

void Plot::_disconnect_from_plot_event(const godot::Ref<Plot_Event> &plot_event) {
    if (!plot_event.is_valid()) return;
    if (plot_event->is_connected("incident_changed", godot::Callable(this, "_plot_event_change"))) {
        plot_event->disconnect("incident_changed", godot::Callable(this, "_plot_event_change"));
    }
}

void Plot::_plot_event_change(const godot::Ref<Plot_Event> &plot_event) {
    evaluate_plot_status();
}

godot::Array Plot::get_narrative_conditions(Narreme *p_narreme) const {
    godot::Array conditions;
    if (p_narreme) {
        godot::String class_name = p_narreme->get_class_name_str();
        if (class_name == "Incident") {
            conditions.append("has plot event incident");
        } else if (class_name == "Plot") {
            conditions.append("is");
        }
    } else {
        conditions.append("not begun");
        conditions.append("in progress");
        conditions.append("complete");
        conditions.append("cannot complete");
        conditions.append("failed");
    }
    return conditions;
}

NarremeConditionStatus Plot::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        godot::String class_name = p_conditional_narreme->get_class_name_str();
        
        if (class_name == "Incident") {
            switch (p_condition_id) {
                case 0:
                    for (int i = 0; i < plot_events.size(); i++) {
                        godot::Ref<Plot_Event> plot_event = plot_events[i];
                        if (plot_event.is_valid() && plot_event->get_incident().ptr() == p_conditional_narreme) {
                            return NarremeConditionStatus::MET;
                        }
                    }
                    return NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Plot") {
            switch (p_condition_id) {
                case 0:
                    return (this == p_conditional_narreme) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } else {
        if ((int)status == p_condition_id) {
            return NarremeConditionStatus::MET;
        } else {
            return NarremeConditionStatus::NOT_MET;
        }
    }
    
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Plot::get_class_name_str() const {
    return "Plot";
}

} // namespace ideam::godot_ext