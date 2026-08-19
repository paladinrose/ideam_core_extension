#include "ideam_editor_inspector_plugin.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/core/class_db.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void IdeamEditorInspectorPlugin::_bind_methods() {
#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("get_undo_redo"), &IdeamEditorInspectorPlugin::get_undo_redo);
#endif
}

IdeamEditorInspectorPlugin::IdeamEditorInspectorPlugin() {
}

IdeamEditorInspectorPlugin::~IdeamEditorInspectorPlugin() {
}

#ifdef TOOLS_ENABLED
EditorUndoRedoManager *IdeamEditorInspectorPlugin::get_undo_redo() const {
	// Virtual base returns nullptr; derived inspectors (e.g., IdeamGraphInspector) 
	// will provide the actual EditorUndoRedoManager instance.
	return nullptr;
}
#endif

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
	if (prop.get_type() != Variant::NIL) {
		// 1. Create the new IdeamUndoRedo instance
		IdeamUndoRedo *ideam_ur = memnew(IdeamUndoRedo);

#ifdef TOOLS_ENABLED
		// 2. Fetch the EditorUndoRedoManager from the plugin
		EditorUndoRedoManager *editor_manager = get_undo_redo();
		if (editor_manager) {
			// 3. Assign it to the IdeamUndoRedo instance
			ideam_ur->set_editor_undo_redo(editor_manager);
		}
#endif

		// 4. Inject the newly created IdeamUndoRedo into the inspected object
		p_object->set("undo_redo", ideam_ur);
	}
}

} // namespace ideam::godot_ext