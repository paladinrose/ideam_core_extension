#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include "../narreme.h" // Needed for NarremeConditionStatus

namespace ideam::godot_ext {

class Incident_Condition : public godot::RefCounted {
    GDCLASS(Incident_Condition, godot::RefCounted)

private:
    godot::String title;
    NarremeConditionStatus last_status = NarremeConditionStatus::UNKNOWN;

protected:
    bool _is_ready = false;
    static void _bind_methods();

public:
    Incident_Condition();
    ~Incident_Condition();

    virtual void initialize(); // Replaces _ready()

    // Getters & Setters
    void set_title(const godot::String &p_title);
    godot::String get_title() const;

    void set_last_status(NarremeConditionStatus p_status);
    NarremeConditionStatus get_last_status() const;

    // DOD NOTE: Virtual functions cause vtable lookups and pipeline stalls. 
    // In IDEAM Core, we will replace polymorphic condition evaluation with 
    // data-driven 'Strategy' execution mapped over contiguous SoA arrays.
    virtual NarremeConditionStatus evaluate_condition();
};

} // namespace ideam::godot_ext