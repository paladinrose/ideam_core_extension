#include "memory_graph_node_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

MemoryGraphNodeResource::MemoryGraphNodeResource() {
    memory_grant.instantiate();
}

void MemoryGraphNodeResource::_bind_methods() {
    godot::ClassDB::bind_integer_constant(get_class_static(), "GrantDerivationMode", "MODE_INDEPENDENT", MODE_INDEPENDENT);
    godot::ClassDB::bind_integer_constant(get_class_static(), "GrantDerivationMode", "MODE_FORKED", MODE_FORKED);

    godot::ClassDB::bind_method(godot::D_METHOD("set_type_id", "id"), &MemoryGraphNodeResource::set_type_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type_id"), &MemoryGraphNodeResource::get_type_id);

    godot::ClassDB::bind_method(godot::D_METHOD("set_derivation_mode", "mode"), &MemoryGraphNodeResource::set_derivation_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_derivation_mode"), &MemoryGraphNodeResource::get_derivation_mode);

    godot::ClassDB::bind_method(godot::D_METHOD("set_memory_grant", "grant"), &MemoryGraphNodeResource::set_memory_grant);
    godot::ClassDB::bind_method(godot::D_METHOD("get_memory_grant"), &MemoryGraphNodeResource::get_memory_grant);

    ADD_GROUP("DOD Constraints", "");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type_id"), "set_type_id", "get_type_id");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "derivation_mode", godot::PROPERTY_HINT_ENUM, "Independent,Forked"), "set_derivation_mode", "get_derivation_mode");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "memory_grant", godot::PROPERTY_HINT_RESOURCE_TYPE, "MemoryGrantResource"), "set_memory_grant", "get_memory_grant");
}

void MemoryGraphNodeResource::set_type_id(uint32_t p_id) { 
    if (p_id == type_id) return;
    type_id = p_id;
    emit_changed();
}
uint32_t MemoryGraphNodeResource::get_type_id() const { return type_id; }

void MemoryGraphNodeResource::set_derivation_mode(int p_mode) {
    if (p_mode == derivation_mode) return;
    derivation_mode = static_cast<GrantDerivationMode>(p_mode);
    emit_changed();
}
int MemoryGraphNodeResource::get_derivation_mode() const { return static_cast<int>(derivation_mode); }

void MemoryGraphNodeResource::set_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    if (memory_grant == p_grant) return; 
    memory_grant = p_grant; 
    emit_changed();
    //godot::UtilityFunctions::print("MemoryGraphNodeResource: Memory grant set to ", memory_grant.is_valid() ? memory_grant->get_grant_name() : "null");
}
godot::Ref<MemoryGrantResource> MemoryGraphNodeResource::get_memory_grant() const { return memory_grant; }

bool MemoryGraphNodeResource::validate_for_compilation() const {
    if (!IdeamGraphNodeResource::validate_for_compilation()) {
        return false;
    }

    if (derivation_mode == MODE_INDEPENDENT) {
        if (!memory_grant.is_valid()) {
            return false; 
        }
        if (!memory_grant->validate_for_compilation()) {
            return false;
        }
    }
    // If MODE_FORKED, we bypass checking the local memory grant because the 
    // compiler will invoke MemoryGraphDOD::fork_grant from a valid parent instead.

    return true;
}

} // namespace ideam::godot_ext