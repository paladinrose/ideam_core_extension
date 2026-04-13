#include "prop.h"
#include <godot_cpp/core/class_db.hpp>

#include "character.h"
#include "location.h" 

namespace ideam::godot_ext {

void Prop::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_possessor", "possessor"), &Prop::set_possessor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_possessor"), &Prop::get_possessor);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "possessor", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character"), "set_possessor", "get_possessor");

    godot::ClassDB::bind_method(godot::D_METHOD("set_current_location", "current_location"), &Prop::set_current_location);
    godot::ClassDB::bind_method(godot::D_METHOD("get_current_location"), &Prop::get_current_location);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location"), "set_current_location", "get_current_location");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("taken_by", "new_possessor"), &Prop::taken_by);
    godot::ClassDB::bind_method(godot::D_METHOD("released_by", "old_possessor"), &Prop::released_by);
    godot::ClassDB::bind_method(godot::D_METHOD("become_lost"), &Prop::become_lost);
    godot::ClassDB::bind_method(godot::D_METHOD("found_by", "finder"), &Prop::found_by);
    godot::ClassDB::bind_method(godot::D_METHOD("enter_location", "new_location"), &Prop::enter_location);
    godot::ClassDB::bind_method(godot::D_METHOD("leave_current_location"), &Prop::leave_current_location);
    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Prop::get_class_name_str);

    // Signals
    ADD_SIGNAL(godot::MethodInfo("taken", godot::PropertyInfo(godot::Variant::OBJECT, "takenBy", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("released", godot::PropertyInfo(godot::Variant::OBJECT, "releasedBy", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("became_lost"));
    ADD_SIGNAL(godot::MethodInfo("found", godot::PropertyInfo(godot::Variant::OBJECT, "foundBy", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("location_entered", godot::PropertyInfo(godot::Variant::OBJECT, "new_location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("location_exited", godot::PropertyInfo(godot::Variant::OBJECT, "previous_location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
}

Prop::Prop() {}

Prop::~Prop() {}

void Prop::initialize() {
    Narreme::initialize();
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS
// ----------------------------------------------------------------------------

void Prop::set_possessor(const godot::Ref<Character> &p_possessor) { possessor = p_possessor; }
godot::Ref<Character> Prop::get_possessor() const { return possessor; }

void Prop::set_current_location(const godot::Ref<Location> &p_location) { current_location = p_location; }
godot::Ref<Location> Prop::get_current_location() const { return current_location; }


// ----------------------------------------------------------------------------
// POSSESSION MANAGEMENT
// ----------------------------------------------------------------------------

void Prop::taken_by(const godot::Ref<Character> &new_possessor) {
    if (!possessor.is_valid()) {
        found_by(new_possessor);
        return;
    }
    
    if (possessor != new_possessor) {
        possessor = new_possessor;
        emit_signal("taken", possessor);
    }
}

void Prop::released_by(const godot::Ref<Character> &old_possessor) {
    if (old_possessor == possessor) {
        possessor.unref();
        emit_signal("released", old_possessor);
    }
}

void Prop::become_lost() {
    if (possessor.is_valid()) {
        emit_signal("released", possessor);
    }
    
    possessor.unref();
    emit_signal("became_lost");
}

void Prop::found_by(const godot::Ref<Character> &finder) {
    possessor = finder;
    emit_signal("found", finder);
}

// ----------------------------------------------------------------------------
// LOCATION MANAGEMENT
// ----------------------------------------------------------------------------

void Prop::enter_location(const godot::Ref<Location> &new_location) {
    if (new_location == current_location) return;
    
    if (current_location.is_valid()) {
        leave_current_location();
    }
    
    current_location = new_location;
    emit_signal("location_entered", current_location);
}

void Prop::leave_current_location() {
    if (current_location.is_valid()) {
        emit_signal("location_exited", current_location);
        
        // Storing old_location to mirror GDScript intent, though it goes unused 
        // in the original script right after this line.
        godot::Ref<Location> old_location = current_location; 
        current_location.unref();
    }
}


// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES
// ----------------------------------------------------------------------------

godot::Array Prop::get_narrative_conditions(Narreme *p_narreme) const {
    godot::Array conditions;
    
    if (p_narreme != nullptr) {
        godot::String class_name = p_narreme->get_class_name_str();
        
        if (class_name == "Character") {
            conditions.append("is possessed by");
            conditions.append("is at the same location as");
        } else if (class_name == "Prop") {
            conditions.append("is");
            conditions.append("has the same possessor as");
            conditions.append("is at the same location as");
        } else if (class_name == "Location") {
            conditions.append("is at");
        }
    } else {
        conditions.append("is lost");
        conditions.append("has possessor");
        conditions.append("is somewhere");
    }
    
    return conditions;
}

NarremeConditionStatus Prop::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    // DOD NOTE: When migrating to the MemoryManagerDOD[cite: 14], this entire 
    // branching structure should be vectorized. Resolving relational matching via 
    // string comparisons and dynamic casts stalls the CPU pipeline. 
    // Instead, we will intersect 'Selection Data' (Bitsets) using AVX2/SSE 
    // for high-speed SIMD Collision checks[cite: 7].
    
    if (p_conditional_narreme) {
        godot::String class_name = p_conditional_narreme->get_class_name_str();
        
        if (class_name == "Character") {
            Character *conditional_character = godot::Object::cast_to<Character>(p_conditional_narreme);
            if (!conditional_character) return NarremeConditionStatus::UNKNOWN;
            
            switch (p_condition_id) {
                case 0:
                    return (possessor.ptr() == conditional_character) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (conditional_character->get_current_location() == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Prop") {
            Prop *conditional_prop = godot::Object::cast_to<Prop>(p_conditional_narreme);
            if (!conditional_prop) return NarremeConditionStatus::UNKNOWN;
            
            switch (p_condition_id) {
                case 0:
                    return (conditional_prop == this) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (conditional_prop->get_possessor() == possessor) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 2:
                    return (conditional_prop->get_current_location() == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Location") {
            // Assume Location is valid if passed in
            godot::Ref<Location> conditional_location(p_conditional_narreme);
            
            switch (p_condition_id) {
                case 0:
                    return (conditional_location == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } 
    else {
        switch (p_condition_id) {
            case 0:
                return (!possessor.is_valid() && !current_location.is_valid()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1:
                return (possessor.is_valid()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2:
                return (current_location.is_valid()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }
    
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Prop::get_class_name_str() const {
    return "Prop";
}

} // namespace ideam::godot_ext