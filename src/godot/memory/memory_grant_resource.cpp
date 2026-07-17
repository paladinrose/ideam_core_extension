#include "memory_grant_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void MemoryGrantResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_integer_constant(get_class_static(), "GrantCapacity", "CAPACITY_LITE", CAPACITY_LITE);
    ClassDB::bind_integer_constant(get_class_static(), "GrantCapacity", "CAPACITY_HEAVY", CAPACITY_HEAVY);

    // Grant Unique Identifiers
    ClassDB::bind_method(D_METHOD("set_grant_name", "name"), &MemoryGrantResource::set_grant_name);
    ClassDB::bind_method(D_METHOD("get_grant_name"), &MemoryGrantResource::get_grant_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "grant_name"), "set_grant_name", "get_grant_name");

    ClassDB::bind_method(D_METHOD("set_capacity_mode", "mode"), &MemoryGrantResource::set_capacity_mode);
    ClassDB::bind_method(D_METHOD("get_capacity_mode"), &MemoryGrantResource::get_capacity_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "capacity_mode", PROPERTY_HINT_ENUM, "Lite (4 Parts),Heavy (8 Parts)"), "set_capacity_mode", "get_capacity_mode");

    ClassDB::bind_method(D_METHOD("set_configured_parts", "parts"), &MemoryGrantResource::set_configured_parts);
    ClassDB::bind_method(D_METHOD("get_configured_parts"), &MemoryGrantResource::get_configured_parts);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "configured_parts", PROPERTY_HINT_ARRAY_TYPE, "GrantPartResource"), "set_configured_parts", "get_configured_parts");

    ClassDB::bind_method(D_METHOD("get_buffer_ids"), &MemoryGrantResource::get_buffer_ids);

    ClassDB::bind_method(D_METHOD("add_part", "part"), &MemoryGrantResource::add_part);
    ClassDB::bind_method(D_METHOD("remove_part", "index"), &MemoryGrantResource::remove_part);
    ClassDB::bind_method(D_METHOD("clear_parts"), &MemoryGrantResource::clear_parts);
    
    // Expose Linter state to Godot Editor GUI
    ClassDB::bind_method(D_METHOD("clear_emulation_state"), &MemoryGrantResource::clear_emulation_state);
    ClassDB::bind_method(D_METHOD("add_emulation_error", "error"), &MemoryGrantResource::add_emulation_error);
    ClassDB::bind_method(D_METHOD("is_emulated_valid"), &MemoryGrantResource::is_emulated_valid);
    ClassDB::bind_method(D_METHOD("get_emulation_errors"), &MemoryGrantResource::get_emulation_errors);
    
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emulated_valid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "is_emulated_valid");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "emulation_errors", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_emulation_errors");

    ClassDB::bind_method(D_METHOD("validate_for_compilation"), &MemoryGrantResource::validate_for_compilation);
}

void MemoryGrantResource::set_grant_name(const godot::StringName& p_name) { 
    if (grant_name == p_name) return;
    grant_name = p_name;
    emit_changed();
}
godot::StringName MemoryGrantResource::get_grant_name() const { return grant_name; }

void MemoryGrantResource::set_capacity_mode(int p_mode) {
    if (p_mode == capacity_mode) return;
    capacity_mode = static_cast<GrantCapacity>(p_mode);
    emit_changed();
}
int MemoryGrantResource::get_capacity_mode() const { return static_cast<int>(capacity_mode); }

void MemoryGrantResource::set_configured_parts(const godot::TypedArray<GrantPartResource>& p_parts) {
    if (p_parts == configured_parts) return;
    configured_parts = p_parts;
    emit_changed();
}
godot::TypedArray<GrantPartResource> MemoryGrantResource::get_configured_parts() const { return configured_parts; }

godot::PackedInt32Array MemoryGrantResource::get_buffer_ids() const {
    godot::PackedInt32Array ids;
    ids.resize(configured_parts.size()); 
    
    for (int i = 0; i < configured_parts.size(); ++i) {
        godot::Ref<GrantPartResource> part = configured_parts[i];
        if (part.is_valid()) {
            ids.set(i, part->get_buffer_id());
        } else {
            ids.set(i, 0xFFFFFFFF); 
        }
    }
    return ids;
}

bool MemoryGrantResource::add_part(const godot::Ref<GrantPartResource>& p_part) {
    if (configured_parts.size() >= static_cast<int>(capacity_mode)) {
        return false;
    }
    if (p_part.is_valid()) {
        configured_parts.push_back(p_part);
        emit_changed();
        return true;
    }
    return false;
}

void MemoryGrantResource::remove_part(int p_index) {
    if (p_index < 0 || p_index >= configured_parts.size()) return;
    configured_parts.remove_at(p_index);
    emit_changed();
}

void MemoryGrantResource::clear_parts() {
    configured_parts.clear();
    emit_changed();
}

void MemoryGrantResource::clear_emulation_state() {
    emulated_valid = true;
    emulation_errors.clear();
}

void MemoryGrantResource::add_emulation_error(const godot::String& p_error) {
    emulated_valid = false;
    emulation_errors.push_back(p_error);
}

bool MemoryGrantResource::validate_for_compilation() const {
    if (configured_parts.size() > static_cast<int>(capacity_mode)) {
        return false;
    }

    for (int i = 0; i < configured_parts.size(); ++i) {
        godot::Ref<GrantPartResource> part = configured_parts[i];
        if (!part.is_valid() || part->get_buffer_id() == 0xFFFFFFFF) {
            return false;
        }
    }
    return true;
}

} // namespace ideam::godot_ext