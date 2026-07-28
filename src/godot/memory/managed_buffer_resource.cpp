#include "managed_buffer_resource.h"

namespace ideam::godot_ext {

void ManagedBufferResource::_bind_methods() {
    using namespace godot;
    
    ClassDB::bind_method(D_METHOD("set_consumer_name", "name"), &ManagedBufferResource::set_consumer_name);
    ClassDB::bind_method(D_METHOD("get_consumer_name"), &ManagedBufferResource::get_consumer_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "consumer_name"), "set_consumer_name", "get_consumer_name");

    ClassDB::bind_method(D_METHOD("set_purpose", "purpose"), &ManagedBufferResource::set_purpose);
    ClassDB::bind_method(D_METHOD("get_purpose"), &ManagedBufferResource::get_purpose);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "purpose"), "set_purpose", "get_purpose");

    ClassDB::bind_method(D_METHOD("set_layout_type", "type"), &ManagedBufferResource::set_layout_type);
    ClassDB::bind_method(D_METHOD("get_layout_type"), &ManagedBufferResource::get_layout_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_type"), "set_layout_type", "get_layout_type");

    ClassDB::bind_method(D_METHOD("set_byte_footprint", "bytes"), &ManagedBufferResource::set_byte_footprint);
    ClassDB::bind_method(D_METHOD("get_byte_footprint"), &ManagedBufferResource::get_byte_footprint);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "byte_footprint"), "set_byte_footprint", "get_byte_footprint");

    ClassDB::bind_method(D_METHOD("set_alignment", "alignment"), &ManagedBufferResource::set_alignment);
    ClassDB::bind_method(D_METHOD("get_alignment"), &ManagedBufferResource::get_alignment);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment"), "set_alignment", "get_alignment");
}

void ManagedBufferResource::set_consumer_name(const godot::StringName& p_name) { 
    if (p_name == consumer_name) return;
    consumer_name = p_name; 
    emit_changed(); 
}
godot::StringName ManagedBufferResource::get_consumer_name() const { return consumer_name; }

void ManagedBufferResource::set_purpose(const godot::StringName& p_purpose) { 
    if (purpose == p_purpose) return;
    purpose = p_purpose; 
    emit_changed(); 
}
godot::StringName ManagedBufferResource::get_purpose() const { return purpose; }

void ManagedBufferResource::set_layout_type(int p_type) { 
    if (layout_type == p_type) return;
    layout_type = p_type; 
    emit_changed(); 
}
int ManagedBufferResource::get_layout_type() const { return layout_type; }

void ManagedBufferResource::set_byte_footprint(int p_bytes) { 
    if (byte_footprint == p_bytes) return;
    byte_footprint = p_bytes; 
    emit_changed(); 
}
int ManagedBufferResource::get_byte_footprint() const { return byte_footprint; }

void ManagedBufferResource::set_alignment(int p_alignment) { 
    if (alignment == p_alignment) return;
    alignment = p_alignment; 
    emit_changed(); 
}
int ManagedBufferResource::get_alignment() const { return alignment; }

} // namespace ideam::godot_ext