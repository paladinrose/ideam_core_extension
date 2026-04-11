#include "ideam_graph_inspector.h"
#include "ideam_graph_edit.h"
#include "ideam_graph_resource.h"
#include "graph_composer.h" 

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

void IdeamGraphInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_edit_graph_pressed", "object"), &IdeamGraphInspector::_on_edit_graph_pressed);
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
    
    return p_object->is_class("IdeamGraphResource");
}

bool IdeamGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    // We anchor the "Edit Graph" button to the 'nodes' array property so it 
    // appears at the very top of the resource's configuration list.
    if (p_name == "nodes") {
        Button *open_button = memnew(Button);
        open_button->set_text("Edit Graph");

        Array args;
        args.append(p_object);
        
        open_button->connect("pressed", Callable(this, "_on_edit_graph_pressed").bindv(args));
        
        add_custom_control(open_button);
        
        // Return false because we still want the 'nodes' array to be visible and editable
        return false; 
    }
    
    return false;
}

void IdeamGraphInspector::_on_edit_graph_pressed(Object* p_object) {
    // 1. Validate and cast the incoming pointer
    auto* raw_resource = Object::cast_to<ideam::godot_ext::IdeamGraphResource>(p_object);
    if (!raw_resource) return;

    // Elevate to a Ref<> to ensure the resource isn't garbage collected while we pass it around
    Ref<ideam::godot_ext::IdeamGraphResource> blueprint(raw_resource);

    // 2. Instantiate the focused editor node
    IdeamGraphEdit* graph_edit = memnew(IdeamGraphEdit);
    
    // Inject the resource blueprint into the editor (assuming setter exists)
    // graph_edit->set_blueprint(blueprint);

    // 3. Unified Routing Pass
    // Bypasses the EditorPlugin singleton entirely. Safe for both Editor and Runtime.
    GraphComposer::edit_ideam_graph(graph_edit);
}

} // namespace godot