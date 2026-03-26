#include "ideam_graph_inspector.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

void IdeamGraphInspector::_bind_methods() {
	// No unique methods to bind currently; logic is handled via overrides.
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
	// Check if the object is an Ideam_Graph or derived
	// Using is_class for C++ defined types
	return p_object->is_class("Ideam_Graph");
}

bool IdeamGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
	if (p_name == "start_node") {
		Button *open_button = memnew(Button);
		open_button->set_text("Edit Graph");

		// We use a lambda-based Callable to bridge to the static IdeamGraphsPlugin method.
		// This replaces the incorrect callable_mp_static header.
		Array args;
		args.append(p_object);
		args.append(Callable()); // Matching the expected p_graph_close argument
		
		open_button->connect("pressed", Callable(IdeamGraphsPlugin::get_singleton(), "edit_ideam_graph").bindv(args));

		add_custom_control(open_button);
	}

	// Return false to ensure the original "start_node" property editor is still drawn
	return false;
}

} // namespace godot