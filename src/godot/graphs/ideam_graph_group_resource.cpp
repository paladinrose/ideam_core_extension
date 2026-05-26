#include "ideam_graph_group_resource.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void IdeamGraphGroupResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_group_name", "name"), &IdeamGraphGroupResource::set_group_name);
    ClassDB::bind_method(D_METHOD("get_group_name"), &IdeamGraphGroupResource::get_group_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "group_name"), "set_group_name", "get_group_name");

    ClassDB::bind_method(D_METHOD("set_title", "title"), &IdeamGraphGroupResource::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &IdeamGraphGroupResource::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_position", "position"), &IdeamGraphGroupResource::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &IdeamGraphGroupResource::get_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "position"), "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("set_size", "size"), &IdeamGraphGroupResource::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &IdeamGraphGroupResource::get_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size"), "set_size", "get_size");

    ClassDB::bind_method(D_METHOD("set_nodes", "nodes"), &IdeamGraphGroupResource::set_nodes);
    ClassDB::bind_method(D_METHOD("get_nodes"), &IdeamGraphGroupResource::get_nodes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_ARRAY_TYPE, "StringName"), "set_nodes", "get_nodes");
}

void IdeamGraphGroupResource::set_group_name(const godot::StringName& p_name) { 
    group_name = p_name; 
}

godot::StringName IdeamGraphGroupResource::get_group_name() const { 
    return group_name; 
}

void IdeamGraphGroupResource::set_title(const godot::String& p_title) { 
    title = p_title; 
}

godot::String IdeamGraphGroupResource::get_title() const { 
    return title; 
}

void IdeamGraphGroupResource::set_position(const godot::Vector2& p_pos) { 
    position = p_pos; 
}

godot::Vector2 IdeamGraphGroupResource::get_position() const { 
    return position; 
}

void IdeamGraphGroupResource::set_size(const godot::Vector2& p_size) { 
    size = p_size; 
}

godot::Vector2 IdeamGraphGroupResource::get_size() const { 
    return size; 
}

void IdeamGraphGroupResource::set_nodes(const godot::TypedArray<godot::StringName>& p_nodes) { 
    nodes = p_nodes; 
}

godot::TypedArray<godot::StringName> IdeamGraphGroupResource::get_nodes() const { 
    return nodes; 
}

} // namespace ideam::godot_ext