#include "grant_part_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GrantPartResource::_bind_methods() {
    using namespace godot;

    // Buffer ID
    ClassDB::bind_method(D_METHOD("set_buffer_id", "buffer_id"), &GrantPartResource::set_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_id"), &GrantPartResource::get_buffer_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "buffer_id"), "set_buffer_id", "get_buffer_id");

    // Element Stride
    ClassDB::bind_method(D_METHOD("set_element_stride", "stride"), &GrantPartResource::set_element_stride);
    ClassDB::bind_method(D_METHOD("get_element_stride"), &GrantPartResource::get_element_stride);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "element_stride"), "set_element_stride", "get_element_stride");

    // Column ID
    ClassDB::bind_method(D_METHOD("set_column_id", "column_id"), &GrantPartResource::set_column_id);
    ClassDB::bind_method(D_METHOD("get_column_id"), &GrantPartResource::get_column_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "column_id"), "set_column_id", "get_column_id");

    ClassDB::bind_method(D_METHOD("set_buffer_type", "type"), &GrantPartResource::set_buffer_type);
    ClassDB::bind_method(D_METHOD("get_buffer_type"), &GrantPartResource::get_buffer_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "buffer_type"), "set_buffer_type", "get_buffer_type");

    // Editor Utility StringNames
    ClassDB::bind_method(D_METHOD("set_target_buffer_name", "name"), &GrantPartResource::set_target_buffer_name);
    ClassDB::bind_method(D_METHOD("get_target_buffer_name"), &GrantPartResource::get_target_buffer_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_buffer_name"), "set_target_buffer_name", "get_target_buffer_name");

    ClassDB::bind_method(D_METHOD("set_target_column_name", "name"), &GrantPartResource::set_target_column_name);
    ClassDB::bind_method(D_METHOD("get_target_column_name"), &GrantPartResource::get_target_column_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_column_name"), "set_target_column_name", "get_target_column_name");

    // Access Mode (Enum Hint: 0=READ, 1=WRITE)
    ClassDB::bind_method(D_METHOD("set_access_mode", "mode"), &GrantPartResource::set_access_mode);
    ClassDB::bind_method(D_METHOD("get_access_mode"), &GrantPartResource::get_access_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "access_mode", PROPERTY_HINT_ENUM, "Read,Write"), "set_access_mode", "get_access_mode");

    // Contiguous Flag
    ClassDB::bind_method(D_METHOD("set_is_contiguous", "is_contiguous"), &GrantPartResource::set_is_contiguous);
    ClassDB::bind_method(D_METHOD("get_is_contiguous"), &GrantPartResource::get_is_contiguous);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_contiguous"), "set_is_contiguous", "get_is_contiguous");

    // Selection
    ClassDB::bind_method(D_METHOD("set_selection", "selection"), &GrantPartResource::set_selection);
    ClassDB::bind_method(D_METHOD("get_selection"), &GrantPartResource::get_selection);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "selection", PROPERTY_HINT_RESOURCE_TYPE, "MemoryBufferSelectionResource"), "set_selection", "get_selection");
}

void GrantPartResource::set_buffer_id(int p_id) { 
    if (buffer_id == static_cast<uint32_t>(p_id)) return;
    buffer_id = static_cast<uint32_t>(p_id); 
    emit_changed();
}
int GrantPartResource::get_buffer_id() const { return static_cast<int>(buffer_id); }

void GrantPartResource::set_element_stride(int p_stride) { 
    if (element_stride == static_cast<uint32_t>(p_stride)) return;
    element_stride = static_cast<uint32_t>(p_stride); 
    emit_changed();
}
int GrantPartResource::get_element_stride() const { return static_cast<int>(element_stride); }

void GrantPartResource::set_column_id(int p_id) { 
    if (column_id == static_cast<uint32_t>(p_id)) return;
    column_id = static_cast<uint32_t>(p_id); 
    emit_changed();
}
int GrantPartResource::get_column_id() const { return static_cast<int>(column_id); }

void GrantPartResource::set_buffer_type(int p_type) { 
    if (buffer_type == static_cast<uint32_t>(p_type)) return;
    buffer_type = static_cast<uint32_t>(p_type);
    emit_changed();
}
int GrantPartResource::get_buffer_type() const { return static_cast<int>(buffer_type);}

void GrantPartResource::set_access_mode(int p_mode) { 
    if (access_mode == p_mode) return;
    access_mode = p_mode; 
    emit_changed();
}
int GrantPartResource::get_access_mode() const { return access_mode; }

void GrantPartResource::set_is_contiguous(bool p_contiguous) { 
    if (is_contiguous == p_contiguous) return;
    is_contiguous = p_contiguous; 
    emit_changed();
}
bool GrantPartResource::get_is_contiguous() const { return is_contiguous; }

void GrantPartResource::set_target_buffer_name(const godot::StringName& p_name) {
    if (p_name == target_buffer_name) return;
    target_buffer_name = p_name; 
    emit_changed();
}
godot::StringName GrantPartResource::get_target_buffer_name() const { return target_buffer_name; }

void GrantPartResource::set_target_column_name(const godot::StringName& p_name) {
    if (target_column_name == p_name) return;
    target_column_name = p_name; 
    emit_changed();
}
godot::StringName GrantPartResource::get_target_column_name() const { return target_column_name; }

void GrantPartResource::set_selection(const godot::Ref<MemoryBufferSelectionResource>& p_selection) {
    if (selection == p_selection) return;
    selection = p_selection;
    emit_changed();
}

godot::Ref<MemoryBufferSelectionResource> GrantPartResource::get_selection() const { 
    return selection; 
}

} // namespace ideam::godot_ext