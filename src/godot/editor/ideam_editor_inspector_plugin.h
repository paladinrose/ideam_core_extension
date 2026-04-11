#ifndef IDEAM_EDITOR_INSPECTOR_PLUGIN_H
#define IDEAM_EDITOR_INSPECTOR_PLUGIN_H

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

class IdeamEditorInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(IdeamEditorInspectorPlugin, EditorInspectorPlugin)

private:
	static void _bind_methods();

public:
	IdeamEditorInspectorPlugin();
	~IdeamEditorInspectorPlugin();

	virtual bool _can_handle(Object *p_object);
	virtual void _parse_begin(Object *p_object) override;

    virtual Object *get_undo_redo() const;
};

} // namespace godot

#endif // IDEAM_EDITOR_INSPECTOR_PLUGIN_H