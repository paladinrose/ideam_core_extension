#include "narrative_condition.h"
#include <godot_cpp/core/class_db.hpp>
#include "../narreme.h" // We need the full Narreme to call check_narrative_condition

namespace ideam::godot_ext {

void Narrative_Condition::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_subject", "subject"), &Narrative_Condition::set_subject);
    godot::ClassDB::bind_method(godot::D_METHOD("get_subject"), &Narrative_Condition::get_subject);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "subject", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narreme"), "set_subject", "get_subject");

    godot::ClassDB::bind_method(godot::D_METHOD("set_object", "object"), &Narrative_Condition::set_object);
    godot::ClassDB::bind_method(godot::D_METHOD("get_object"), &Narrative_Condition::get_object);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "object", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narreme"), "set_object", "get_object");

    godot::ClassDB::bind_method(godot::D_METHOD("set_condition", "condition"), &Narrative_Condition::set_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("get_condition"), &Narrative_Condition::get_condition);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "condition"), "set_condition", "get_condition");

    godot::ClassDB::bind_method(godot::D_METHOD("set_subject_is_symbolic", "subject_is_symbolic"), &Narrative_Condition::set_subject_is_symbolic);
    godot::ClassDB::bind_method(godot::D_METHOD("get_subject_is_symbolic"), &Narrative_Condition::get_subject_is_symbolic);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "subject_is_symbolic"), "set_subject_is_symbolic", "get_subject_is_symbolic");

    godot::ClassDB::bind_method(godot::D_METHOD("set_object_is_symbolic", "object_is_symbolic"), &Narrative_Condition::set_object_is_symbolic);
    godot::ClassDB::bind_method(godot::D_METHOD("get_object_is_symbolic"), &Narrative_Condition::get_object_is_symbolic);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "object_is_symbolic"), "set_object_is_symbolic", "get_object_is_symbolic");

    ADD_SIGNAL(godot::MethodInfo("subject_changed", godot::PropertyInfo(godot::Variant::OBJECT, "narcon", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")));
    ADD_SIGNAL(godot::MethodInfo("condition_changed", godot::PropertyInfo(godot::Variant::OBJECT, "narcon", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")));
    ADD_SIGNAL(godot::MethodInfo("object_changed", godot::PropertyInfo(godot::Variant::OBJECT, "narcon", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")));
}

Narrative_Condition::Narrative_Condition() {}
Narrative_Condition::~Narrative_Condition() {}

void Narrative_Condition::initialize() {
    if (_is_ready) return;
    Incident_Condition::initialize();
}

void Narrative_Condition::set_subject(const godot::Ref<Narreme> &p_subject) {
    if (_subject == p_subject) return;
    if (!_is_ready) {
        _subject = p_subject;
        return;
    }
    _subject = p_subject;
    emit_signal("subject_changed", this);
}
godot::Ref<Narreme> Narrative_Condition::get_subject() const { return _subject; }

void Narrative_Condition::set_object(const godot::Ref<Narreme> &p_object) {
    if (_object == p_object) return;
    if (!_is_ready) {
        _object = p_object;
        return;
    }
    _object = p_object;
    emit_signal("object_changed", this);
}
godot::Ref<Narreme> Narrative_Condition::get_object() const { return _object; }

void Narrative_Condition::set_condition(int p_condition) {
    if (_condition == p_condition) return;
    if (!_is_ready) {
        _condition = p_condition;
        return;
    }
    _condition = p_condition;
    emit_signal("condition_changed", this);
}
int Narrative_Condition::get_condition() const { return _condition; }

void Narrative_Condition::set_subject_is_symbolic(bool p_val) { subject_is_symbolic = p_val; }
bool Narrative_Condition::get_subject_is_symbolic() const { return subject_is_symbolic; }

void Narrative_Condition::set_object_is_symbolic(bool p_val) { object_is_symbolic = p_val; }
bool Narrative_Condition::get_object_is_symbolic() const { return object_is_symbolic; }

NarremeConditionStatus Narrative_Condition::evaluate_condition() {
    if (!_subject.is_valid()) return NarremeConditionStatus::CANNOT_MEET;
    
    NarremeConditionStatus new_status = _subject->check_narrative_condition(_condition, _object.ptr());
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