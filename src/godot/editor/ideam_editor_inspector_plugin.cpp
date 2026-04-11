#include "ideam_editor_inspector_plugin.h"

#include <godot_cpp/core/class_db.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void IdeamEditorInspectorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_undo_redo"), &IdeamEditorInspectorPlugin::get_undo_redo);
}

IdeamEditorInspectorPlugin::IdeamEditorInspectorPlugin() {
}

IdeamEditorInspectorPlugin::~IdeamEditorInspectorPlugin() {
}

Object *IdeamEditorInspectorPlugin::get_undo_redo() const {
	// Virtual base returns nullptr; derived inspectors (e.g., IdeamGraphInspector) 
	// will provide the actual EditorUndoRedoManager instance.
	return nullptr;
}

bool IdeamEditorInspectorPlugin::_can_handle(Object *p_object) {
	// Base plugin handles general Ideam reflection; specialized logic belongs in inherited classes.
	// Returning true here allows the property check to run for any inspected object.
	return true;
}

void IdeamEditorInspectorPlugin::_parse_begin(Object *p_object) {
	if (!p_object) {
		return;
	}

	// Check if the object has an 'undo_redo' property for injection
	Variant prop = p_object->get("undo_redo");
	
	// We verify the property exists by checking if the result is not NIL
	// and then attempt to assign the current EditorUndoRedoManager.
	if (prop.get_type() != Variant::NIL) {
		Object *ur = get_undo_redo();
		if (ur) {
			p_object->set("undo_redo", ur);
		}
	}
}

} // namespace ideam::godot_ext