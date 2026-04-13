#ifndef TASK_GRAPH_INSPECTOR_H
#define TASK_GRAPH_INSPECTOR_H

#include "../editor/ideam_editor_inspector_plugin.h"
#include "../graphs/ideam_graphs_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/object.hpp>

namespace ideam::godot_ext {

class TaskGraphInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(TaskGraphInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();
    void _on_edit_graph_pressed(godot::Object* p_object);

public:
    TaskGraphInspector();
    virtual ~TaskGraphInspector() override;

    virtual godot::Object *get_undo_redo() const override;
    virtual bool _can_handle(godot::Object *p_object) override;
    virtual bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override;
    virtual void _parse_begin(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext

#endif // TASK_GRAPH_INSPECTOR_H