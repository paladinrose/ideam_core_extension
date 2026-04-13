#include "task_graph_inspector.h"
#include "task_graph_edit.h"
#include "task_graph_resource.h"
#include "ideam_tasks_plugin.h"
#include "../graphs/graph_composer.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void TaskGraphInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_edit_graph_pressed", "object"), &TaskGraphInspector::_on_edit_graph_pressed);
}

TaskGraphInspector::TaskGraphInspector() {}
TaskGraphInspector::~TaskGraphInspector() {}

Object *TaskGraphInspector::get_undo_redo() const {
    return IdeamTasksPlugin::undo_redo();
}

bool TaskGraphInspector::_can_handle(Object *p_object) {
    if (!p_object) return false;
    return Object::cast_to<TaskGraphResource>(p_object) != nullptr;
}

void TaskGraphInspector::_parse_begin(Object *p_object) {
    Button *open_button = memnew(Button);
    open_button->set_text("Edit Task Graph");

    Array args;
    args.append(p_object);
    open_button->connect("pressed", Callable(this, "_on_edit_graph_pressed").bindv(args));
    
    add_custom_control(open_button);
}

bool TaskGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    return false;
}

void TaskGraphInspector::_on_edit_graph_pressed(Object* p_object) {
    auto* raw_resource = Object::cast_to<TaskGraphResource>(p_object);
    if (!raw_resource) return;

    // Secure the resource blueprint
    Ref<TaskGraphResource> blueprint(raw_resource);

    // Instantiate the specific TaskGraphEdit editor node
    TaskGraphEdit* graph_edit = memnew(TaskGraphEdit);
    
    // Inject the blueprint using the exact interface parity established by the core graph
    graph_edit->set_blueprint(blueprint);
    
    // Delegate strictly to the GraphComposer engine via the Graphs Plugin
    GraphComposer::edit_ideam_graph(graph_edit);
}

} // namespace ideam::godot_ext