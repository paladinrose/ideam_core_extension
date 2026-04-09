#include "managed_buffer_profile.h"

namespace ideam::godot_ext {

void ManagedBufferProfile::_bind_methods() {
    using namespace godot;
    
    ClassDB::bind_method(D_METHOD("set_consumer_name", "name"), &ManagedBufferProfile::set_consumer_name);
    ClassDB::bind_method(D_METHOD("get_consumer_name"), &ManagedBufferProfile::get_consumer_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "consumer_name"), "set_consumer_name", "get_consumer_name");

    ClassDB::bind_method(D_METHOD("set_purpose", "purpose"), &ManagedBufferProfile::set_purpose);
    ClassDB::bind_method(D_METHOD("get_purpose"), &ManagedBufferProfile::get_purpose);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "purpose"), "set_purpose", "get_purpose");

    ClassDB::bind_method(D_METHOD("set_layout_type", "type"), &ManagedBufferProfile::set_layout_type);
    ClassDB::bind_method(D_METHOD("get_layout_type"), &ManagedBufferProfile::get_layout_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_type"), "set_layout_type", "get_layout_type");

    ClassDB::bind_method(D_METHOD("set_byte_footprint", "bytes"), &ManagedBufferProfile::set_byte_footprint);
    ClassDB::bind_method(D_METHOD("get_byte_footprint"), &ManagedBufferProfile::get_byte_footprint);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "byte_footprint"), "set_byte_footprint", "get_byte_footprint");

    ClassDB::bind_method(D_METHOD("set_alignment", "alignment"), &ManagedBufferProfile::set_alignment);
    ClassDB::bind_method(D_METHOD("get_alignment"), &ManagedBufferProfile::get_alignment);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment"), "set_alignment", "get_alignment");
}

} // namespace ideam::godot_ext