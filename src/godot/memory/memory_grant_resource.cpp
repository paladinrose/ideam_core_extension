#include "memory_grant_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void MemoryGrantResource::_bind_methods() {
    godot::ClassDB::bind_integer_constant(get_class_static(), "GrantCapacity", "CAPACITY_LITE", CAPACITY_LITE);
    godot::ClassDB::bind_integer_constant(get_class_static(), "GrantCapacity", "CAPACITY_HEAVY", CAPACITY_HEAVY);

    godot::ClassDB::bind_method(godot::D_METHOD("set_capacity_mode", "mode"), &MemoryGrantResource::set_capacity_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_capacity_mode"), &MemoryGrantResource::get_capacity_mode);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "capacity_mode", godot::PROPERTY_HINT_ENUM, "Lite (4 Parts),Heavy (8 Parts)"), "set_capacity_mode", "get_capacity_mode");

    godot::ClassDB::bind_method(godot::D_METHOD("set_configured_parts", "parts"), &MemoryGrantResource::set_configured_parts);
    godot::ClassDB::bind_method(godot::D_METHOD("get_configured_parts"), &MemoryGrantResource::get_configured_parts);
    
    // Enforce array typing in the Godot Inspector
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "configured_parts", godot::PROPERTY_HINT_ARRAY_TYPE, "GrantPartResource"), "set_configured_parts", "get_configured_parts");

    godot::ClassDB::bind_method(godot::D_METHOD("add_part", "part"), &MemoryGrantResource::add_part);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_part", "index"), &MemoryGrantResource::remove_part);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_parts"), &MemoryGrantResource::clear_parts);
    
    godot::ClassDB::bind_method(godot::D_METHOD("validate_for_compilation"), &MemoryGrantResource::validate_for_compilation);
}

void MemoryGrantResource::set_capacity_mode(int p_mode) {
    capacity_mode = static_cast<GrantCapacity>(p_mode);
}

int MemoryGrantResource::get_capacity_mode() const {
    return static_cast<int>(capacity_mode);
}

void MemoryGrantResource::set_configured_parts(const godot::TypedArray<GrantPartResource>& p_parts) {
    configured_parts = p_parts;
}

godot::TypedArray<GrantPartResource> MemoryGrantResource::get_configured_parts() const {
    return configured_parts;
}

bool MemoryGrantResource::add_part(const godot::Ref<GrantPartResource>& p_part) {
    if (configured_parts.size() >= capacity_mode) {
        // Reject the UI addition to guarantee we never overflow our compile-time DOD struct limits.
        return false;
    }
    
    if (p_part.is_valid()) {
        configured_parts.push_back(p_part);
        return true;
    }
    return false;
}

void MemoryGrantResource::remove_part(int p_index) {
    if (p_index >= 0 && p_index < configured_parts.size()) {
        configured_parts.remove_at(p_index);
    }
}

void MemoryGrantResource::clear_parts() {
    configured_parts.clear();
}

bool MemoryGrantResource::validate_for_compilation() const {
    // 1. Array Bound Validation
    if (configured_parts.size() > capacity_mode) {
        return false;
    }

    // 2. State & Integrity Validation
    for (int i = 0; i < configured_parts.size(); ++i) {
        godot::Ref<GrantPartResource> part = configured_parts[i];
        if (!part.is_valid()) {
            return false; // Prevent nullptrs from crashing the DOD compilation
        }
        
        // Example check: Reject unassigned buffer targets
        if (part->get_buffer_id() == 0xFFFFFFFF) {
            return false;
        }
    }

    return true;
}

} // namespace ideam::godot_ext