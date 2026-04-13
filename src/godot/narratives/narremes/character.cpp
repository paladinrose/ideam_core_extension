#include "character.h"
#include <godot_cpp/core/class_db.hpp>

#include "location.h"
#include "prop.h"
#include "incident.h"
#include "plot.h"
#include "narrative.h"
#include "../helpers/relationship.h"

namespace ideam::godot_ext {

void Character::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_current_location", "current_location"), &Character::set_current_location);
    godot::ClassDB::bind_method(godot::D_METHOD("get_current_location"), &Character::get_current_location);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location"), "set_current_location", "get_current_location");

    godot::ClassDB::bind_method(godot::D_METHOD("set_possessions", "possessions"), &Character::set_possessions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_possessions"), &Character::get_possessions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "possessions", godot::PROPERTY_HINT_ARRAY_TYPE, "Prop"), "set_possessions", "get_possessions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_relationships", "relationships"), &Character::set_relationships);
    godot::ClassDB::bind_method(godot::D_METHOD("get_relationships"), &Character::get_relationships);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "relationships", godot::PROPERTY_HINT_ARRAY_TYPE, "Relationship"), "set_relationships", "get_relationships");

    godot::ClassDB::bind_method(godot::D_METHOD("set_goals", "goals"), &Character::set_goals);
    godot::ClassDB::bind_method(godot::D_METHOD("get_goals"), &Character::get_goals);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "goals", godot::PROPERTY_HINT_ARRAY_TYPE, "Plot"), "set_goals", "get_goals");

    // Location Methods
    godot::ClassDB::bind_method(godot::D_METHOD("enter_location", "newLocation"), &Character::enter_location);
    godot::ClassDB::bind_method(godot::D_METHOD("leave_current_location"), &Character::leave_current_location);

    // Possession Methods
    godot::ClassDB::bind_method(godot::D_METHOD("take_possession", "possession"), &Character::take_possession);
    godot::ClassDB::bind_method(godot::D_METHOD("get_possession_id", "possession"), &Character::get_possession_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_possession_of", "possession"), &Character::has_possession_of);
    godot::ClassDB::bind_method(godot::D_METHOD("lose_possession", "possession"), &Character::lose_possession);
    godot::ClassDB::bind_method(godot::D_METHOD("lose_possession_at", "id"), &Character::lose_possession_at);

    // Goal Methods
    godot::ClassDB::bind_method(godot::D_METHOD("set_goal", "goal"), &Character::set_goal);
    godot::ClassDB::bind_method(godot::D_METHOD("get_goal_id", "goal"), &Character::get_goal_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_goal", "goal"), &Character::has_goal);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_goal", "goal"), &Character::remove_goal);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_goal_at", "goal_ID"), &Character::remove_goal_at);
    godot::ClassDB::bind_method(godot::D_METHOD("check_goal_status", "goal"), &Character::check_goal_status);
    godot::ClassDB::bind_method(godot::D_METHOD("check_goal_status_at", "goal_ID"), &Character::check_goal_status_at);
    godot::ClassDB::bind_method(godot::D_METHOD("goal_success", "goal_ID"), &Character::goal_success);
    godot::ClassDB::bind_method(godot::D_METHOD("goal_fail", "goal_ID"), &Character::goal_fail);

    // Relationship Methods
    godot::ClassDB::bind_method(godot::D_METHOD("begin_relationship", "relation", "title"), &Character::begin_relationship);
    godot::ClassDB::bind_method(godot::D_METHOD("change_relation", "id", "newRelation"), &Character::change_relation);
    godot::ClassDB::bind_method(godot::D_METHOD("change_relationship", "id", "newTitle"), &Character::change_relationship);
    godot::ClassDB::bind_method(godot::D_METHOD("get_relationship_id", "title"), &Character::get_relationship_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_relation_ids", "target"), &Character::get_relation_ids);

    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Character::get_class_name_str);

    // Signals
    ADD_SIGNAL(godot::MethodInfo("location_entered", godot::PropertyInfo(godot::Variant::OBJECT, "new_location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("location_exited", godot::PropertyInfo(godot::Variant::OBJECT, "location_exited", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("possession_taken", godot::PropertyInfo(godot::Variant::OBJECT, "new_possession", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("possession_lost", godot::PropertyInfo(godot::Variant::OBJECT, "lost_possession", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("already_has_possession", godot::PropertyInfo(godot::Variant::OBJECT, "possession", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("relationship_started", godot::PropertyInfo(godot::Variant::OBJECT, "new_relationship", godot::PROPERTY_HINT_RESOURCE_TYPE, "Relationship")));
    ADD_SIGNAL(godot::MethodInfo("relationship_ended", godot::PropertyInfo(godot::Variant::OBJECT, "ended_relationship", godot::PROPERTY_HINT_RESOURCE_TYPE, "Relationship")));
    ADD_SIGNAL(godot::MethodInfo("relation_changed", godot::PropertyInfo(godot::Variant::OBJECT, "relationship", godot::PROPERTY_HINT_RESOURCE_TYPE, "Relationship"), godot::PropertyInfo(godot::Variant::OBJECT, "old_relation", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narreme")));
    ADD_SIGNAL(godot::MethodInfo("relationship_changed", godot::PropertyInfo(godot::Variant::OBJECT, "relationship", godot::PROPERTY_HINT_RESOURCE_TYPE, "Relationship"), godot::PropertyInfo(godot::Variant::STRING, "old_title")));
    ADD_SIGNAL(godot::MethodInfo("new_goal_set", godot::PropertyInfo(godot::Variant::OBJECT, "new_goal", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("goal_achieved", godot::PropertyInfo(godot::Variant::OBJECT, "achieved_goal", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("goal_failed", godot::PropertyInfo(godot::Variant::OBJECT, "failed_goal", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("goal_removed", godot::PropertyInfo(godot::Variant::OBJECT, "goal", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
}

Character::Character() {}

Character::~Character() {}

void Character::initialize() {
    Narreme::initialize();
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS
// ----------------------------------------------------------------------------

void Character::set_current_location(const godot::Ref<Location> &p_location) { current_location = p_location; }
godot::Ref<Location> Character::get_current_location() const { return current_location; }

void Character::set_possessions(const godot::TypedArray<Prop> &p_possessions) { possessions = p_possessions; }
godot::TypedArray<Prop> Character::get_possessions() const { return possessions; }

void Character::set_relationships(const godot::TypedArray<Relationship> &p_relationships) { relationships = p_relationships; }
godot::TypedArray<Relationship> Character::get_relationships() const { return relationships; }

void Character::set_goals(const godot::TypedArray<Plot> &p_goals) { goals = p_goals; }
godot::TypedArray<Plot> Character::get_goals() const { return goals; }


// ----------------------------------------------------------------------------
// LOCATION MANAGEMENT
// ----------------------------------------------------------------------------

void Character::enter_location(const godot::Ref<Location> &p_new_location) {
    if (p_new_location == current_location) return;
    
    if (current_location.is_valid()) {
        leave_current_location();
    }
    
    current_location = p_new_location;
    emit_signal("location_entered", current_location);
}

void Character::leave_current_location() {
    if (!current_location.is_valid()) return;
    
    emit_signal("location_exited", current_location);
    current_location.unref();
}

// ----------------------------------------------------------------------------
// POSSESSION MANAGEMENT
// ----------------------------------------------------------------------------

int Character::take_possession(const godot::Ref<Prop> &p_possession) {
    int id = get_possession_id(p_possession);
    
    if (id >= 0) {
        emit_signal("already_has_possession", p_possession);
    } else {
        id = possessions.size();
        possessions.append(p_possession);
        emit_signal("possession_taken", p_possession);
    }
    
    return id;
}

int Character::get_possession_id(const godot::Ref<Prop> &p_possession) const {
    return possessions.find(p_possession);
}

bool Character::has_possession_of(const godot::Ref<Prop> &p_possession) const {
    return possessions.has(p_possession);
}

void Character::lose_possession(const godot::Ref<Prop> &p_possession) {
    int id = get_possession_id(p_possession);
    lose_possession_at(id);
}

void Character::lose_possession_at(int p_id) {
    if (p_id < 0 || p_id >= possessions.size()) return;
    
    godot::Ref<Prop> possession = possessions[p_id];
    possessions.remove_at(p_id);
    emit_signal("possession_lost", possession);
}

// ----------------------------------------------------------------------------
// GOAL MANAGEMENT
// ----------------------------------------------------------------------------

int Character::set_goal(const godot::Ref<Plot> &p_goal) {
    int goal_id = get_goal_id(p_goal);
    
    if (goal_id < 0) {
        goal_id = goals.size();
        goals.append(p_goal);
        emit_signal("new_goal_set", p_goal);
    }
    
    return goal_id;
}

int Character::get_goal_id(const godot::Ref<Plot> &p_goal) const {
    return goals.find(p_goal);
}

bool Character::has_goal(const godot::Ref<Plot> &p_goal) const {
    return goals.has(p_goal);
}

void Character::remove_goal(const godot::Ref<Plot> &p_goal) {
    int goal_id = get_goal_id(p_goal);
    remove_goal_at(goal_id);
}

void Character::remove_goal_at(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) return;
    
    godot::Ref<Plot> goal = goals[p_goal_id];
    goals.remove_at(p_goal_id);
    emit_signal("goal_removed", goal);
}

int Character::check_goal_status(const godot::Ref<Plot> &p_goal) {
    return check_goal_status_at(get_goal_id(p_goal));
}

int Character::check_goal_status_at(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) {
        // Assume CANNOT_COMPLETE is a defined state, using generic -1 for fallback 
        // until Plot class is fully ported.
        return -1; 
    }
    
    godot::Ref<Plot> goal = goals[p_goal_id];
    if (!goal.is_valid()) return -1;

    // DOD NOTE: This requires Plot to be fully implemented with `get_status` 
    // and `evaluate_plot_status`. These constants match standard GDScript enum intent.
    int status = goal->call("get_status");
    
    // Assuming IN_PROGRESS = 1, NOT_BEGUN = 0 from PlotStatus
    if (status != 1 && status != 0) {
        return status;
    }
    
    status = goal->call("evaluate_plot_status");
    
    // Assuming CANNOT_COMPLETE = -1, FAILED = -2, COMPLETE = 2
    switch (status) {
        case -1:
        case -2:
            goal_fail(p_goal_id);
            break;
        case 2:
            goal_success(p_goal_id);
            break;
    }
    
    return status;
}

void Character::goal_success(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) return;
    emit_signal("goal_achieved", goals[p_goal_id]);
}

void Character::goal_fail(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) return;
    emit_signal("goal_failed", goals[p_goal_id]);
}

// ----------------------------------------------------------------------------
// RELATIONSHIP MANAGEMENT
// ----------------------------------------------------------------------------

int Character::begin_relationship(const godot::Ref<Narreme> &p_relation, const godot::String &p_title) {
    int id = get_relationship_id(p_title);
    if (id < 0) {
        // DOD NOTE: Creating `new` objects dynamically like this fragments the heap.
        // In IDEAM Core, relationships won't be distinct allocations, but contiguous 
        // rows in a relational BridgeView mapping Table.
        godot::Ref<Relationship> new_relationship;
        new_relationship.instantiate(); 
        
        // Assumes Relationship has these setters
        new_relationship->call("set_relation", p_relation);
        new_relationship->call("set_title", p_title);
        
        id = relationships.size();
        relationships.append(new_relationship);
        emit_signal("relationship_started", new_relationship);
    } 
    else {
        godot::Ref<Relationship> existing_rel = relationships[id];
        if (existing_rel->get_relation() != p_relation) {
            if (!change_relation(id, p_relation)) {
                return -1;
            }
        }
    }
    return id;
}

bool Character::change_relation(int p_id, const godot::Ref<Narreme> &p_new_relation) {
    if (p_id < 0 || p_id >= relationships.size()) return false;
    
    godot::Ref<Relationship> rel = relationships[p_id];
    godot::Ref<Narreme> old_relation = rel->call("get_relation");
    
    if (old_relation == p_new_relation) return false;
    
    rel->call("set_relation", p_new_relation);
    emit_signal("relation_changed", rel, old_relation);
    return true;
}

bool Character::change_relationship(int p_id, const godot::String &p_new_title) {
    if (p_id < 0 || p_id >= relationships.size()) return false;
    
    godot::Ref<Relationship> rel = relationships[p_id];
    godot::String old_title = rel->call("get_title");
    
    if (old_title == p_new_title) return false;
    
    rel->call("set_title", p_new_title);
    emit_signal("relationship_changed", rel, old_title);
    return true;
}

int Character::get_relationship_id(const godot::String &p_title) const {
    for (int i = 0; i < relationships.size(); i++) {
        godot::Ref<Relationship> rel = relationships[i];
        if (rel.is_valid() && rel->call("get_title") == p_title) {
            return i;
        }
    }
    return -1;
}

godot::TypedArray<int> Character::get_relation_ids(const godot::Ref<Narreme> &p_target) const {
    godot::TypedArray<int> ids;
    for (int i = 0; i < relationships.size(); i++) {
        godot::Ref<Relationship> rel = relationships[i];
        if (rel.is_valid() && rel->get_relation() == p_target) {
            ids.append(i);
        }
    }
    return ids;
}


// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES
// ----------------------------------------------------------------------------

godot::Array Character::get_narrative_conditions(Narreme *p_narreme) const {
    godot::Array conditions;
    
    if (p_narreme != nullptr) {
        godot::String class_name = p_narreme->get_class_name_str();
        
        if (class_name == "Character") {
            conditions.append("is");
            conditions.append("is at the same location as");
            conditions.append("has the same goal as");
            conditions.append("has a relationship with");
        } else if (class_name == "Prop") {
            conditions.append("possesses");
            conditions.append("has a relationship with");
        } else if (class_name == "Location") {
            conditions.append("is at");
            conditions.append("has a relationship with");
        } else if (class_name == "Incident") {
            conditions.append("has a relationship with");
        } else if (class_name == "Plot") {
            conditions.append("has goal");
            conditions.append("has a relationship with");
        } else if (class_name == "Narrative") {
            conditions.append("has a relationship with");
        }
    } else {
        conditions.append("is somewhere.");
        conditions.append("has possessions.");
        conditions.append("has relationships.");
        conditions.append("has goals.");
    }
    
    return conditions;
}

NarremeConditionStatus Character::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        godot::String class_name = p_conditional_narreme->get_class_name_str();
        
        if (class_name == "Character") {
            Character *conditional_character = godot::Object::cast_to<Character>(p_conditional_narreme);
            if (!conditional_character) return NarremeConditionStatus::UNKNOWN;
            
            switch (p_condition_id) {
                case 0:
                    return (conditional_character == this) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (conditional_character->get_current_location() == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 2: {
                    // DOD NOTE: SIMD intersection target. Nested pointer loops destroy cache lines.
                    godot::TypedArray<Plot> other_goals = conditional_character->get_goals();
                    for (int i = 0; i < goals.size(); i++) {
                        for (int j = 0; j < other_goals.size(); j++) {
                            if (goals[i] == other_goals[j]) {
                                return NarremeConditionStatus::MET;
                            }
                        }
                    }
                    return NarremeConditionStatus::NOT_MET;
                }
                case 3:
                    return (get_relation_ids(conditional_character).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Prop") {
            // Assume Prop exists
            godot::Ref<Prop> conditional_prop(p_conditional_narreme);
            switch (p_condition_id) {
                case 0:
                    return has_possession_of(conditional_prop) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Location") {
            godot::Ref<Location> conditional_location(p_conditional_narreme);
            switch (p_condition_id) {
                case 0:
                    return (conditional_location == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Incident") {
            switch (p_condition_id) {
                case 0:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Plot") {
            godot::Ref<Plot> conditional_plot(p_conditional_narreme);
            switch (p_condition_id) {
                case 0:
                    return (get_goal_id(conditional_plot) >= 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } 
        else if (class_name == "Narrative") {
            switch (p_condition_id) {
                case 0:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } 
    else {
        switch (p_condition_id) {
            case 0:
                return current_location.is_valid() ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1:
                return (possessions.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2:
                return (relationships.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 3:
                return (goals.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }
    
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Character::get_class_name_str() const {
    return "Character";
}

} // namespace ideam::godot_ext