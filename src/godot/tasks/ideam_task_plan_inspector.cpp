#include "ideam_task_plan_inspector.h"
#include "ideam_tasks_plugin.h"
#include "../graphs/ideam_graphs_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void IdeamTaskPlanInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_edit_plan_pressed", "plan_object"), &IdeamTaskPlanInspector::_on_edit_plan_pressed);
}

bool IdeamTaskPlanInspector::_can_handle(Object *p_object) {
    if (!p_object) return false;
    // Mirroring 'if object is Task_Plan'
    return p_object->is_class("Task_Plan") || p_object->get_class() == "Task_Plan";
}

Object *IdeamTaskPlanInspector::get_undo_redo() const {
    return IdeamTasksPlugin::undo_redo();
}

void IdeamTaskPlanInspector::_parse_begin(Object *p_object) {
    // 1. Call base to handle 'undo_redo' property injection on the Plan itself
    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    // 2. Deep injection for sub-tasks
    Object *ur = get_undo_redo();
    if (ur) {
        Variant tasks_var = p_object->get("tasks");
        if (tasks_var.get_type() == Variant::ARRAY) {
            Array tasks = tasks_var;
            for (int i = 0; i < tasks.size(); ++i) {
                Object *task_obj = tasks[i];
                if (task_obj) {
                    task_obj->set("undo_redo", ur);
                }
            }
        }
    }
}

bool IdeamTaskPlanInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    
    if (p_name == "tasks") {
        Button *open_button = memnew(Button);
        open_button->set_text("Edit Plan");
        
        // Connect the button to our handler, passing the inspected object as an argument
        open_button->connect("pressed", callable_mp(this, &IdeamTaskPlanInspector::_on_edit_plan_pressed).bind(p_object));
        
        add_custom_control(open_button);
        
        // Return true if you want to hide the default array inspector, 
        // false if you want the button to appear ABOVE the array.
        return false; 
    }

    return false;
}

void IdeamTaskPlanInspector::_on_edit_plan_pressed(Object *p_plan_object) {
    if (!p_plan_object) return;

    // 1. Trigger the logic to create the visual graph representation
    Variant graph_var = p_plan_object->call("create_graph");
    Object *graph_obj = graph_var;

    if (graph_obj) {
        Object *ur = get_undo_redo();
        
        // 2. Ensure the new graph and all current tasks have the UndoRedo reference
        p_plan_object->set("undo_redo", ur);
        graph_obj->set("undo_redo", ur);

        Variant tasks_var = p_plan_object->get("tasks");
        if (tasks_var.get_type() == Variant::ARRAY) {
            Array tasks = tasks_var;
            for (int i = 0; i < tasks.size(); ++i) {
                Object *task_obj = tasks[i];
                if (task_obj) {
                    task_obj->set("undo_redo", ur);
                }
            }
        }

        // 3. Hand the graph over to the IdeamGraphsPlugin singleton for visualization
        IdeamGraphsPlugin::edit_graph(graph_obj, Callable());
    }
}

} // namespace godot