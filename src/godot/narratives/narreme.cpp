#include "narreme.h"

namespace ideam::godot_ext {

void Narreme::_bind_methods() {
    // Property Binding
    godot::ClassDB::bind_method(godot::D_METHOD("set_official_name", "official_name"), &Narreme::set_official_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_official_name"), &Narreme::get_official_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "official_name"), "set_official_name", "get_official_name");

    // Method Binding
    godot::ClassDB::bind_method(godot::D_METHOD("initialize"), &Narreme::initialize);
    godot::ClassDB::bind_method(godot::D_METHOD("get_narrative_conditions", "narreme"), &Narreme::get_narrative_conditions, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("check_narrative_condition", "conditionID", "conditionalNarreme"), &Narreme::check_narrative_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("score_similarity", "similar"), &Narreme::score_similarity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Narreme::get_class_name_str);

    // Enum Binding
    BIND_ENUM_CONSTANT(NarremeConditionStatus::CANNOT_MEET);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::NOT_MET);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::UNKNOWN);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::MET);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::PERMANENT);
}

Narreme::Narreme() {
    // DOD NOTE: Keeping constructors clean. Heavy allocations should be avoided 
    // here to prevent heap fragmentation.
}

Narreme::~Narreme() {
}

void Narreme::initialize() {
    // Replaces the GDScript _ready() logic
    if (official_name.is_empty()) {
        official_name = get_name();
    }
}

void Narreme::set_official_name(const godot::String &p_name) {
    official_name = p_name;
}

godot::String Narreme::get_official_name() const {
    return official_name;
}

godot::Array Narreme::get_narrative_conditions(Narreme *p_narreme) const {
    // DOD NOTE: When we transition this to IDEAM's MemoryManagerDOD, 
    // relational queries shouldn't spawn new Godot Arrays. They should return a 
    // `MemoryBufferSelectionPOD` containing metadata and indices[cite: 18].
    return godot::Array();
}

NarremeConditionStatus Narreme::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    return NarremeConditionStatus::UNKNOWN;
}

int Narreme::score_similarity(Narreme *p_similar) const {
    if (!p_similar) return 1;
    
    // DOD NOTE: String comparisons here should eventually be replaced by 
    // O(1) integer checks against Partition IDs [cite: 4] to maintain cache speed.
    if (this->get_class_name_str() == p_similar->get_class_name_str()) {
        return 0;
    }
    return 1;
}

godot::String Narreme::get_class_name_str() const {
    return "Narreme";
}

} // namespace ideam::godot_ext