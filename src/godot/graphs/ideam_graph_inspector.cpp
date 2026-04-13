#include "ideam_graph_inspector.h"
#include "ideam_graph_edit.h"
#include "ideam_graph_resource.h"
#include "graph_composer.h" 

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void IdeamGraphInspector::_bind_methods() {
}

IdeamGraphInspector::IdeamGraphInspector() {
}

IdeamGraphInspector::~IdeamGraphInspector() {
}

Object *IdeamGraphInspector::get_undo_redo() const {
    return IdeamGraphsPlugin::undo_redo();
}

bool IdeamGraphInspector::_can_handle(Object *p_object) {
    if (!p_object) {
        return false;
    }
    return Object::cast_to<IdeamGraphResource>(p_object) != nullptr;
}

bool IdeamGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    return false;
}



} // namespace ideam::godot_ext