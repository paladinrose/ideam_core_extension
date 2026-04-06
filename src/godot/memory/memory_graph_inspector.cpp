#include "memory_graph_inspector.h"
#include "memory_graph_edit.h"
#include "memory_graph_resource.h"
#include "../graphs/graph_composer.h" // Adjust path to GraphComposer

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

void MemoryGraphInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_edit_graph_pressed", "object"), &MemoryGraphInspector::_on_edit_graph_pressed);
}

MemoryGraphInspector::MemoryGraphInspector() {}
MemoryGraphInspector::~MemoryGraphInspector() {}

Object *MemoryGraphInspector::get_undo_redo() const {
    return IdeamGraphsPlugin::undo_redo();
}

bool MemoryGraphInspector::_can_handle(Object *p_object) {
    if (!p_object) return false;
    return p_object->is_class("MemoryGraphResource");
}

bool MemoryGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    // Injecting the localized edit button at the top of the relevant array
    if (p_name == "nodes") {
        Button *open_button = memnew(Button);
        open_button->set_text("Edit Memory Graph");

        Array args;
        args.append(p_object);
        open_button->connect("pressed", Callable(this, "_on_edit_graph_pressed").bindv(args));
        
        add_custom_control(open_button);
        return false; 
    }
    return false;
}

void MemoryGraphInspector::_on_edit_graph_pressed(Object* p_object) {
    auto* raw_resource = Object::cast_to<ideam::godot_ext::MemoryGraphResource>(p_object);
    if (!raw_resource) return;

    // Secure the resource in a Ref<> block to prevent garbage collection during handoff
    Ref<ideam::godot_ext::MemoryGraphResource> blueprint(raw_resource);

    // Instantiate the tightly-coupled viewport node
    MemoryGraphEdit* graph_edit = memnew(MemoryGraphEdit);
    // graph_edit->set_blueprint(blueprint); // Assuming your setter is configured

    // Route to the DOD-optimized static composer manager
    GraphComposer::edit_ideam_graph(graph_edit, nullptr);
}

} // namespace godot