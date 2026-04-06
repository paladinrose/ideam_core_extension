#include "task_graph_node.h"
#include "task_graph_resource.h"

namespace godot {

void TaskGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_task_type"), &TaskGraphNode::get_task_type);
}

void TaskGraphNode::_build_ui() {
    MemoryGraphNode::_build_ui();

    Dictionary props = get_properties();
    
    // Extract task type from the initialized dictionary
    if (props.has("task_type")) {
        task_type = static_cast<uint32_t>(props["task_type"]);
    }

    task_type_label = memnew(Label);
    
    switch (task_type) {
        case ideam::godot_ext::TASK_GODOT_REFLECTION:
            task_type_label->set_text("Type: GDScript Reflection");
            set_self_modulate(Color(0.5f, 0.5f, 1.0f)); // Blue-ish
            break;
        case ideam::godot_ext::TASK_NATIVE_CPU:
            task_type_label->set_text("Type: Native CPU Transform");
            set_self_modulate(Color(1.0f, 0.5f, 0.5f)); // Red-ish
            break;
        case ideam::godot_ext::TASK_COMPUTE_GPU:
            task_type_label->set_text("Type: GPU Compute Shader");
            set_self_modulate(Color(0.8f, 0.3f, 0.8f)); // Purple-ish
            break;
        case ideam::godot_ext::TASK_QUERY_CULLER:
            task_type_label->set_text("Type: Query / Culler");
            set_self_modulate(Color(1.0f, 0.8f, 0.2f)); // Yellow/Orange
            break;
        default:
            task_type_label->set_text("Type: Unknown");
            break;
    }

    // Add visual indicator to the top of the node
    add_child(task_type_label);
    move_child(task_type_label, 0); 
}

} // namespace godot