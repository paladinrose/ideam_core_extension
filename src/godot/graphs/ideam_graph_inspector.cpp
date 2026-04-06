#include "ideam_graph_inspector.h"

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
	// Proxies the singleton from the plugin as defined in the GDScript logic
	return IdeamGraphsPlugin::undo_redo();
}

bool IdeamGraphInspector::_can_handle(Object *p_object) {
	if (!p_object) {
		return false;
	}
	
	// Retargeted from "Ideam_Graph" to our new DOD serialization resource
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
		
		// Replaced the custom lambda with a standard ClassDB bound method for GDExtension safety
		open_button->connect("pressed", Callable(this, "_on_edit_graph_pressed").bindv(args));
		
		add_custom_control(open_button);
		
		// Return false because we still want the 'nodes' array to be visible and editable
		return false; 
	}
	
	return false;
}

void IdeamGraphInspector::_on_edit_graph_pressed(Object* p_object) {
	// In GDExtension, since IdeamGraphsPlugin is instantiated by the Editor, 
	// we route the UI request to the global scope or invoke via reflection.
	
	// Note: If you add `static IdeamGraphsPlugin* get_singleton()` to the plugin header,
	// you can directly call: IdeamGraphsPlugin::get_singleton()->edit_ideam_graph(p_object, Callable());
	
	godot::UtilityFunctions::print("Ideam: Opening Graph Composer for ", p_object);
}

} // namespace godot