#ifndef MEMORY_GRAPH_INSPECTOR_H
#define MEMORY_GRAPH_INSPECTOR_H

#include "../editor/ideam_editor_inspector_plugin.h"
#include "../graphs/ideam_graphs_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot {

class MemoryGraphInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(MemoryGraphInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();
    void _on_edit_graph_pressed(Object* p_object);

public:
    MemoryGraphInspector();
    virtual ~MemoryGraphInspector() override;

    virtual Object *get_undo_redo() const override;
    virtual bool _can_handle(Object *p_object) override;
    virtual bool _parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) override;
};

} // namespace godot

#endif // MEMORY_GRAPH_INSPECTOR_H