#ifndef IDEAM_TASK_PLAN_INSPECTOR_H
#define IDEAM_TASK_PLAN_INSPECTOR_H

#include "../editor/ideam_editor_inspector_plugin.h"
#include <godot_cpp/classes/button.hpp>

namespace godot {

/**
 * @class IdeamTaskPlanInspector
 * @brief Inspector for Task_Plan resources.
 * Injects UndoRedo and provides a bridge to the Graph Composer.
 */
class IdeamTaskPlanInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(IdeamTaskPlanInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    IdeamTaskPlanInspector() = default;
    virtual ~IdeamTaskPlanInspector() override = default;

    virtual bool _can_handle(Object *p_object) override;
    virtual void _parse_begin(Object *p_object) override;
    virtual bool _parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) override;

    // Implementation of base virtual to provide the manager
    virtual Object *get_undo_redo() const override;

    // Button Callback
    void _on_edit_plan_pressed(Object *p_plan_object);
};

} // namespace godot

#endif // IDEAM_TASK_PLAN_INSPECTOR_H