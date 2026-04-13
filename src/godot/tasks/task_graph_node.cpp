#include "task_graph_node.h"
#include "task_graph_resource.h"

using namespace godot;

namespace ideam::godot_ext {

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
        case TASK_GODOT_REFLECTION:
            task_type_label->set_text("Type: GDScript Reflection");
            set_self_modulate(Color(0.5f, 0.5f, 1.0f)); // Blue-ish
            break;
        case TASK_NATIVE_CPU:
            task_type_label->set_text("Type: Native CPU Transform");
            set_self_modulate(Color(1.0f, 0.5f, 0.5f)); // Red-ish
            break;
        case TASK_COMPUTE_GPU:
            task_type_label->set_text("Type: GPU Compute Shader");
            set_self_modulate(Color(0.8f, 0.3f, 0.8f)); // Purple-ish
            break;
        case TASK_QUERY_CULLER:
            task_type_label->set_text("Type: Query / Culler");
            set_self_modulate(Color(0.3f, 0.8f, 0.3f)); // Green-ish
            break;
        default:
            task_type_label->set_text("Type: Unknown");
            break;
    }
    
    add_child(task_type_label);
}

} // namespace ideam::godot_ext