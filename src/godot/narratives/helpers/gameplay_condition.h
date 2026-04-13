#pragma once

#include "incident_condition.h"

namespace ideam::godot_ext {

class Gameplay_Condition : public Incident_Condition {
    GDCLASS(Gameplay_Condition, Incident_Condition)

private:
    NarremeConditionStatus gameplay_status = NarremeConditionStatus::UNKNOWN;

protected:
    static void _bind_methods();

public:
    Gameplay_Condition();
    ~Gameplay_Condition();

    void set_gameplay_status(NarremeConditionStatus p_status);
    NarremeConditionStatus get_gameplay_status() const;

    void set_condition_met();
    void set_condition_not_met();
    void set_condition_unknown();
    void set_condition_permanent();
    void set_condition_cannot_meet();

    virtual NarremeConditionStatus evaluate_condition() override;
};

} // namespace ideam::godot_ext