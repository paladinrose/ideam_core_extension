#include "task_graph_edit.h"
#include "task_graph_resource.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void TaskGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_task_popup_select", "id"), &TaskGraphEdit::_on_task_popup_select);
}

TaskGraphEdit::TaskGraphEdit() {
}

TaskGraphEdit::~TaskGraphEdit() {
}

void TaskGraphEdit::_ready() {
    MemoryGraphEdit::_ready();

    // Re-route the popup selection from the base class to our specialized Task builder
    // We disconnect the base class signal and bind our own
    if (PopupMenu* popup = Object::cast_to<PopupMenu>(find_child("PopupMenu", true, false))) {
        if (popup->is_connected("id_pressed", Callable(this, "_filtered_popup_select"))) {
            popup->disconnect("id_pressed", Callable(this, "_filtered_popup_select"));
        }
        popup->connect("id_pressed", Callable(this, "_on_task_popup_select"));
    }
}

TypedArray<String> TaskGraphEdit::_get_filtered_node_types(uint32_t p_filter_mask) const {
    TypedArray<String> arr;
    
    // For now, we present the raw fundamental task types. 
    // Future implementations will query a C++ Registry for all registered T_Logic names.
    arr.push_back("GDScript Reflection Task");
    arr.push_back("Native CPU Transform Task");
    arr.push_back("GPU Compute Task");
    arr.push_back("Query / Culler Task");
    
    return arr;
}

void TaskGraphEdit::_on_task_popup_select(int p_id) {
    Ref<ideam::godot_ext::IdeamGraphResource> blueprint = get_blueprint();
    if (blueprint.is_null()) return;

    StringName unique_name = String("TaskNode_") + String::num_int64(UtilityFunctions::randi());

    // Inject the TaskType into the properties dictionary so TaskGraphNode can read it
    Dictionary props;
    props["task_type"] = p_id; 

    // Setup defaults
    if (p_id == ideam::godot_ext::TASK_GODOT_REFLECTION) {
        props["execution_method"] = "execute_task";
    }

    Dictionary new_node;
    new_node["name"] = unique_name;
    new_node["type_id"] = p_id; 
    new_node["properties"] = props;
    
    // Adjust for viewport scroll offset 
    // Note: We use get_scroll_offset() inherited from GraphEdit. 
    // memory_popup_position requires accessing protected members, so in practice 
    // you might retrieve the mouse position directly here if memory_popup_position isn't accessible.
    new_node["position"] = get_local_mouse_position() + get_scroll_offset();
    
    blueprint->action_add_node(new_node);
}

} // namespace godot