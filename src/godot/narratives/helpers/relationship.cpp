#include "relationship.h"
#include <godot_cpp/core/class_db.hpp>
#include "../narreme.h" 

namespace ideam::godot_ext {

void Relationship::_bind_methods() {
    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &Relationship::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &Relationship::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_relation", "relation"), &Relationship::set_relation);
    godot::ClassDB::bind_method(godot::D_METHOD("get_relation"), &Relationship::get_relation);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "relation", godot::PROPERTY_HINT_RESOURCE_TYPE, "Narreme"), "set_relation", "get_relation");
}

Relationship::Relationship() {}

Relationship::~Relationship() {}

void Relationship::set_title(const godot::String &p_title) {
    title = p_title;
}

godot::String Relationship::get_title() const {
    return title;
}

void Relationship::set_relation(const godot::Ref<Narreme> &p_relation) {
    relation = p_relation;
}

godot::Ref<Narreme> Relationship::get_relation() const {
    return relation;
}

} // namespace ideam::godot_ext