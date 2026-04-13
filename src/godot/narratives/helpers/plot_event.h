#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

// Forward declaration
class Incident;

enum IncidentRequirement : int8_t {
    INCIDENT_REQ_HAS_NOT_HAPPENED = 0,
    INCIDENT_REQ_HAS_HAPPENED = 1,
    INCIDENT_REQ_CANNOT_HAPPEN = 2,
    INCIDENT_REQ_INSTANT_SUCCESS = 3,
    INCIDENT_REQ_INSTANT_FAILURE = 4
};

class Plot_Event : public godot::RefCounted {
    GDCLASS(Plot_Event, godot::RefCounted)

private:
    // DOD NOTE: In the Authoring layer, this holds a heavy Ref to the Incident.
    // During the DOD "Bake" step, this will serialize down to an integer `Incident_ID`
    // combined with a bitmask representing the `IncidentRequirement`.
    godot::String title;
    godot::Ref<Incident> incident;
    IncidentRequirement requirement = INCIDENT_REQ_HAS_NOT_HAPPENED;

    bool _is_ready = false;
    bool _instant = false;

protected:
    static void _bind_methods();

public:
    Plot_Event();
    ~Plot_Event();

    void initialize(); // Replaces _ready()

    // Getters & Setters
    void set_title(const godot::String &p_title);
    godot::String get_title() const;

    void set_incident(const godot::Ref<Incident> &p_incident);
    godot::Ref<Incident> get_incident() const;

    void set_requirement(IncidentRequirement p_requirement);
    IncidentRequirement get_requirement() const;

    // Methods
    bool check_requirement();
    
    void _connect_to_incident();
    void _disconnect_from_incident();
    
    void _cannot_happen();
    void _happened();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::IncidentRequirement);