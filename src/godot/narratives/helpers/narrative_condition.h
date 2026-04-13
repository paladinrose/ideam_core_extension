#pragma once

#include "incident_condition.h"

namespace ideam::godot_ext {

// Forward Declaration
class Narreme;

class Narrative_Condition : public Incident_Condition {
    GDCLASS(Narrative_Condition, Incident_Condition)

private:
    // DOD NOTE: Subject and Object pointers create severe spatial dispersion.
    // In DOD, conditions become flat bitsets. A SIMD `AOSOAView` will 
    // evaluate thousands of these checks simultaneously without jumping to 
    // random addresses in memory.
    godot::Ref<Narreme> _subject;
    godot::Ref<Narreme> _object;
    int _condition = 0;
    
    bool subject_is_symbolic = false;
    bool object_is_symbolic = false;

protected:
    static void _bind_methods();

public:
    Narrative_Condition();
    ~Narrative_Condition();

    virtual void initialize() override;

    void set_subject(const godot::Ref<Narreme> &p_subject);
    godot::Ref<Narreme> get_subject() const;

    void set_object(const godot::Ref<Narreme> &p_object);
    godot::Ref<Narreme> get_object() const;

    void set_condition(int p_condition);
    int get_condition() const;

    void set_subject_is_symbolic(bool p_val);
    bool get_subject_is_symbolic() const;

    void set_object_is_symbolic(bool p_val);
    bool get_object_is_symbolic() const;

    virtual NarremeConditionStatus evaluate_condition() override;
};

} // namespace ideam::godot_ext