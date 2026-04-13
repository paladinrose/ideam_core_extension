#include "gameplay_condition.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void Gameplay_Condition::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_gameplay_status", "gameplay_status"), &Gameplay_Condition::set_gameplay_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gameplay_status"), &Gameplay_Condition::get_gameplay_status);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "gameplay_status"), "set_gameplay_status", "get_gameplay_status");

    godot::ClassDB::bind_method(godot::D_METHOD("set_condition_met"), &Gameplay_Condition::set_condition_met);
    godot::ClassDB::bind_method(godot::D_METHOD("set_condition_not_met"), &Gameplay_Condition::set_condition_not_met);
    godot::ClassDB::bind_method(godot::D_METHOD("set_condition_unknown"), &Gameplay_Condition::set_condition_unknown);
    godot::ClassDB::bind_method(godot::D_METHOD("set_condition_permanent"), &Gameplay_Condition::set_condition_permanent);
    godot::ClassDB::bind_method(godot::D_METHOD("set_condition_cannot_meet"), &Gameplay_Condition::set_condition_cannot_meet);
}

Gameplay_Condition::Gameplay_Condition() {}
Gameplay_Condition::~Gameplay_Condition() {}

void Gameplay_Condition::set_gameplay_status(NarremeConditionStatus p_status) { gameplay_status = p_status; }
NarremeConditionStatus Gameplay_Condition::get_gameplay_status() const { return gameplay_status; }

void Gameplay_Condition::set_condition_met() {
    NarremeConditionStatus status = get_last_status();
    if (status == NarremeConditionStatus::CANNOT_MEET || status == NarremeConditionStatus::PERMANENT) return;
    gameplay_status = NarremeConditionStatus::MET;
    evaluate_condition();
}

void Gameplay_Condition::set_condition_not_met() {
    NarremeConditionStatus status = get_last_status();
    if (status == NarremeConditionStatus::CANNOT_MEET || status == NarremeConditionStatus::PERMANENT) return;
    gameplay_status = NarremeConditionStatus::NOT_MET;
    evaluate_condition();
}

void Gameplay_Condition::set_condition_unknown() {
    NarremeConditionStatus status = get_last_status();
    if (status == NarremeConditionStatus::CANNOT_MEET || status == NarremeConditionStatus::PERMANENT) return;
    gameplay_status = NarremeConditionStatus::UNKNOWN;
    evaluate_condition();
}

void Gameplay_Condition::set_condition_permanent() {
    NarremeConditionStatus status = get_last_status();
    if (status == NarremeConditionStatus::CANNOT_MEET || status == NarremeConditionStatus::PERMANENT) return;
    gameplay_status = NarremeConditionStatus::PERMANENT;
    evaluate_condition();
}

void Gameplay_Condition::set_condition_cannot_meet() {
    NarremeConditionStatus status = get_last_status();
    if (status == NarremeConditionStatus::CANNOT_MEET || status == NarremeConditionStatus::PERMANENT) return;
    gameplay_status = NarremeConditionStatus::CANNOT_MEET;
    evaluate_condition();
}

NarremeConditionStatus Gameplay_Condition::evaluate_condition() {
    NarremeConditionStatus status = get_last_status();
    
    if (gameplay_status == status) return status;
    if (status == NarremeConditionStatus::CANNOT_MEET) return status;
    if (status == NarremeConditionStatus::PERMANENT) return status;
    
    switch (gameplay_status) {
        case NarremeConditionStatus::UNKNOWN: emit_signal("became_unknown"); break;
        case NarremeConditionStatus::MET: emit_signal("met"); break;
        case NarremeConditionStatus::CANNOT_MEET: emit_signal("cannot_meet"); break;
        case NarremeConditionStatus::NOT_MET: emit_signal("unmet"); break;
        case NarremeConditionStatus::PERMANENT: emit_signal("met"); break;
    }
    
    set_last_status(gameplay_status);
    return gameplay_status;
}

} // namespace ideam::godot_ext