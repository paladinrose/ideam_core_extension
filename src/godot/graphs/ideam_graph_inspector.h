#ifndef IDEAM_GRAPH_INSPECTOR_H
#define IDEAM_GRAPH_INSPECTOR_H

#include "../editor/ideam_editor_inspector_plugin.h"
#include "ideam_graphs_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot {

class IdeamGraphInspector : public IdeamEditorInspectorPlugin {
	GDCLASS(IdeamGraphInspector, IdeamEditorInspectorPlugin)

protected:
	static void _bind_methods();

public:
	IdeamGraphInspector();
	virtual ~IdeamGraphInspector() override;

	// Overrides from IdeamEditorInspectorPlugin
	virtual Object *get_undo_redo() const override;

	// Overrides from EditorInspectorPlugin
	virtual bool _can_handle(Object *p_object) override;
	virtual bool _parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) override;
};

} // namespace godot

#endif // IDEAM_GRAPH_INSPECTOR_H