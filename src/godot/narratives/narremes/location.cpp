#include <godot_cpp/core/class_db.hpp>

#include "location.h"
#include "character.h"
#include "prop.h"

namespace ideam::godot_ext {

void Location::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_characters", "characters"), &Location::set_characters);
    godot::ClassDB::bind_method(godot::D_METHOD("get_characters"), &Location::get_characters);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "characters", godot::PROPERTY_HINT_ARRAY_TYPE, "Character"), "set_characters", "get_characters");

    godot::ClassDB::bind_method(godot::D_METHOD("set_props", "props"), &Location::set_props);
    godot::ClassDB::bind_method(godot::D_METHOD("get_props"), &Location::get_props);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "props", godot::PROPERTY_HINT_ARRAY_TYPE, "Prop"), "set_props", "get_props");

    // Character Methods
    godot::ClassDB::bind_method(godot::D_METHOD("character_entrance", "character"), &Location::character_entrance);
    godot::ClassDB::bind_method(godot::D_METHOD("get_character_id", "character"), &Location::get_character_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_character", "character"), &Location::has_character);
    godot::ClassDB::bind_method(godot::D_METHOD("character_exit", "character"), &Location::character_exit);
    godot::ClassDB::bind_method(godot::D_METHOD("character_exit_at", "character_ID"), &Location::character_exit_at);

    // Prop Methods
    godot::ClassDB::bind_method(godot::D_METHOD("prop_entrance", "prop"), &Location::prop_entrance);
    godot::ClassDB::bind_method(godot::D_METHOD("get_prop_id", "prop"), &Location::get_prop_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_prop", "prop"), &Location::has_prop);
    godot::ClassDB::bind_method(godot::D_METHOD("prop_exit", "prop"), &Location::prop_exit);
    godot::ClassDB::bind_method(godot::D_METHOD("prop_exit_at", "prop_ID"), &Location::prop_exit_at);

    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Location::get_class_name_str);

    // Signals
    ADD_SIGNAL(godot::MethodInfo("character_entered", godot::PropertyInfo(godot::Variant::OBJECT, "character", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("character_exited", godot::PropertyInfo(godot::Variant::OBJECT, "character", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("prop_entered", godot::PropertyInfo(godot::Variant::OBJECT, "prop", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("prop_exited", godot::PropertyInfo(godot::Variant::OBJECT, "prop", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
}

Location::Location() {}

Location::~Location() {}

void Location::initialize() {
    Narreme::initialize();
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS
// ----------------------------------------------------------------------------

void Location::set_characters(const godot::TypedArray<Character> &p_characters) { characters = p_characters; }
godot::TypedArray<Character> Location::get_characters() const { return characters; }

void Location::set_props(const godot::TypedArray<Prop> &p_props) { props = p_props; }
godot::TypedArray<Prop> Location::get_props() const { return props; }


// ----------------------------------------------------------------------------
// CHARACTER MANAGEMENT
// ----------------------------------------------------------------------------

int Location::character_entrance(const godot::Ref<Character> &character) {
    int character_ID = get_character_id(character);
    
    if (character_ID < 0) {
        character_ID = characters.size();
        characters.append(character);
        emit_signal("character_entered", character);
    }
    
    return character_ID;
}

int Location::get_character_id(const godot::Ref<Character> &character) const {
    return characters.find(character);
}

bool Location::has_character(const godot::Ref<Character> &character) const {
    return characters.has(character);
}

bool Location::character_exit(const godot::Ref<Character> &character) {
    int character_ID = get_character_id(character);
    return character_exit_at(character_ID);
}

bool Location::character_exit_at(int character_ID) {
    if (character_ID < 0 || character_ID >= characters.size()) {
        return false;
    }
    
    godot::Ref<Character> toRemove = characters[character_ID];
    characters.remove_at(character_ID);
    emit_signal("character_exited", toRemove);
    
    return true;
}


// ----------------------------------------------------------------------------
// PROP MANAGEMENT
// ----------------------------------------------------------------------------

int Location::prop_entrance(const godot::Ref<Prop> &prop) {
    int prop_ID = get_prop_id(prop);
    
    if (prop_ID < 0) {
        prop_ID = props.size();
        props.append(prop);
        emit_signal("prop_entered", prop);
    }
    
    return prop_ID;
}

int Location::get_prop_id(const godot::Ref<Prop> &prop) const {
    return props.find(prop);
}

bool Location::has_prop(const godot::Ref<Prop> &prop) const {
    return props.has(prop);
}

bool Location::prop_exit(const godot::Ref<Prop> &prop) {
    int prop_ID = get_prop_id(prop);
    return prop_exit_at(prop_ID);
}

bool Location::prop_exit_at(int prop_ID) {
    if (prop_ID < 0 || prop_ID >= props.size()) {
        return false;
    }
    
    godot::Ref<Prop> toRemove = props[prop_ID];
    props.remove_at(prop_ID);
    emit_signal("prop_exited", toRemove);
    
    return true;
}


// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES
// ----------------------------------------------------------------------------

godot::Array Location::get_narrative_conditions(Narreme *p_narreme) const {
    godot::Array conditions;
    
    if (p_narreme != nullptr) {
        godot::String class_name = p_narreme->get_class_name_str();
        
        if (class_name == "Character") {
            conditions.append("has character");
        } else if (class_name == "Prop") {
            conditions.append("has prop");
        } else if (class_name == "Location") {
            conditions.append("is");
        }
    } else {
        conditions.append("is empty");
        conditions.append("has characters present");
        conditions.append("has props present");
    }
    
    return conditions;
}

NarremeConditionStatus Location::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    // DOD NOTE: Condition checking like this implies O(N) array scans through `has_character` 
    // or `has_prop`. In IDEAM, we will evaluate these relationships instantly using AVX2/SSE 
    // bitset intersections via Group Masks or Selection Data[cite: 4, 7].
    
    if (p_conditional_narreme) {
        godot::String class_name = p_conditional_narreme->get_class_name_str();
        
        if (class_name == "Character") {
            godot::Ref<Character> conditional_character(p_conditional_narreme);
            switch (p_condition_id) {
                case 0:
                    return has_character(conditional_character) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Prop") {
            godot::Ref<Prop> conditional_prop(p_conditional_narreme);
            switch (p_condition_id) {
                case 0:
                    return has_prop(conditional_prop) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Location") {
            Location *conditional_location = godot::Object::cast_to<Location>(p_conditional_narreme);
            if (!conditional_location) return NarremeConditionStatus::UNKNOWN;
            
            switch (p_condition_id) {
                case 0:
                    return (conditional_location == this) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } 
    else {
        switch (p_condition_id) {
            case 0:
                return (characters.size() == 0 && props.size() == 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1:
                return (characters.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2:
                return (props.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }
    
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Location::get_class_name_str() const {
    return "Location";
}

} // namespace ideam::godot_ext