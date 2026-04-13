#include "memory_graph_inspector.h"
#include "memory_graph_edit.h"
#include "memory_graph_resource.h"
#include "../graphs/graph_composer.h" 

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void MemoryGraphInspector::_bind_methods() {
}

MemoryGraphInspector::MemoryGraphInspector() {
}

MemoryGraphInspector::~MemoryGraphInspector() {
}

Object *MemoryGraphInspector::get_undo_redo() const {
    // Relying on the shared Graph Plugin undo_redo ensures history isn't fragmented 
    // when modifying the Memory Graph in the composer
    return IdeamMemoryPlugin::undo_redo();
}

bool MemoryGraphInspector::_can_handle(Object *p_object) {
    if (!p_object) return false;
    return Object::cast_to<MemoryGraphResource>(p_object) != nullptr;
}

bool MemoryGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    
    return false;
}



} // namespace ideam::godot_ext