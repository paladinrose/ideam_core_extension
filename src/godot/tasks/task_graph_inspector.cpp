#include "task_graph_inspector.h"
#include "task_graph_edit.h"
#include "task_graph_resource.h"
#include "../graphs/graph_composer.h" // Adjust path to GraphComposer

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

void TaskGraphInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_edit_graph_pressed", "object"), &TaskGraphInspector::_on_edit_graph_pressed);
}

TaskGraphInspector::TaskGraphInspector() {}
TaskGraphInspector::~TaskGraphInspector() {}

Object *TaskGraphInspector::get_undo_redo() const {
    return IdeamGraphsPlugin::undo_redo();
}

bool TaskGraphInspector::_can_handle(Object *p_object) {
    if (!p_object) return false;
    return p_object->is_class("TaskGraphResource");
}

bool TaskGraphInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    if (p_name == "nodes") {
        Button *open_button = memnew(Button);
        open_button->set_text("Edit Task Graph");

        Array args;
        args.append(p_object);
        open_button->connect("pressed", Callable(this, "_on_edit_graph_pressed").bindv(args));
        
        add_custom_control(open_button);
        return false; 
    }
    return false;
}

void TaskGraphInspector::_on_edit_graph_pressed(Object* p_object) {
    auto* raw_resource = Object::cast_to<ideam::godot_ext::TaskGraphResource>(p_object);
    if (!raw_resource) return;

    Ref<ideam::godot_ext::TaskGraphResource> blueprint(raw_resource);

    TaskGraphEdit* graph_edit = memnew(TaskGraphEdit);
    // graph_edit->set_blueprint(blueprint); 

    GraphComposer::edit_ideam_graph(graph_edit, nullptr);
}

} // namespace godot