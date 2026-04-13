#ifndef MEMORY_GRAPH_INSPECTOR_H
#define MEMORY_GRAPH_INSPECTOR_H

#include "../editor/ideam_editor_inspector_plugin.h"
#include "ideam_memory_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/object.hpp>

namespace ideam::godot_ext {

class MemoryGraphInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(MemoryGraphInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

    // Signal receiver for the injected button
    void _on_edit_graph_pressed(godot::Object* p_object);

public:
    MemoryGraphInspector();
    virtual ~MemoryGraphInspector() override;

    // Overrides from IdeamEditorInspectorPlugin
    virtual godot::Object *get_undo_redo() const override;

    // Overrides from EditorInspectorPlugin
    virtual bool _can_handle(godot::Object *p_object) override;
    virtual bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override;
};

} // namespace ideam::godot_ext

#endif // MEMORY_GRAPH_INSPECTOR_H