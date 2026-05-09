#include "memory_graph_node_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

MemoryGraphNodeResource::MemoryGraphNodeResource() {
    // Pre-instantiate the MemoryGrantResource. In an editor environment, 
    // ensuring this is never null by default vastly improves UX and 
    // prevents constant null-reference checking during rapid prototyping.
    memory_grant.instantiate();
}

void MemoryGraphNodeResource::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_type_id", "id"), &MemoryGraphNodeResource::set_type_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type_id"), &MemoryGraphNodeResource::get_type_id);

    godot::ClassDB::bind_method(godot::D_METHOD("set_memory_grant", "grant"), &MemoryGraphNodeResource::set_memory_grant);
    godot::ClassDB::bind_method(godot::D_METHOD("get_memory_grant"), &MemoryGraphNodeResource::get_memory_grant);

    // Grouping this away from the "Editor Visuals" established in the base class.
    // This emphasizes to the user that these are hard execution constraints.
    ADD_GROUP("DOD Constraints", "");
    
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type_id"), "set_type_id", "get_type_id");
    
    // Enforce that the inspector only accepts MemoryGrantResource types
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "memory_grant", godot::PROPERTY_HINT_RESOURCE_TYPE, "MemoryGrantResource"), "set_memory_grant", "get_memory_grant");
}

void MemoryGraphNodeResource::set_type_id(uint32_t p_id) {
    type_id = p_id;
}

uint32_t MemoryGraphNodeResource::get_type_id() const {
    return type_id;
}

void MemoryGraphNodeResource::set_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    memory_grant = p_grant;
}

godot::Ref<MemoryGrantResource> MemoryGraphNodeResource::get_memory_grant() const {
    return memory_grant;
}

bool MemoryGraphNodeResource::validate_for_compilation() const {
    // 1. Validate Base Identity
    // The underlying ideam_graph_dod requires a topological identity to map connections.
    if (!IdeamGraphNodeResource::validate_for_compilation()) {
        return false;
    }

    // 2. Validate Data-Oriented Memory Constraints
    // A MemoryGraphNode MUST have a valid memory grant declaration to participate in the simulation.
    if (!memory_grant.is_valid()) {
        return false; 
    }

    // 3. Delegate to the Grant's internal hardware-limit validation
    // (e.g., ensuring we haven't exceeded the N=4 or N=8 bounds of our cache lines)
    if (!memory_grant->validate_for_compilation()) {
        return false;
    }

    return true;
}

} // namespace ideam::godot_ext