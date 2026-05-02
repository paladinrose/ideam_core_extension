#include "task_graph_node.h"
#include "task_graph_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/theme.hpp>

using namespace godot;

namespace ideam::godot_ext {

TaskGraphNode::TaskGraphNode() {
}

void TaskGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_task_type"), &TaskGraphNode::get_task_type);
    ClassDB::bind_method(D_METHOD("get_logic_id"), &TaskGraphNode::get_logic_id);
    ClassDB::bind_method(D_METHOD("get_logic_name"), &TaskGraphNode::get_logic_name);
    
    ClassDB::bind_method(D_METHOD("_on_custom_param_changed", "param_name", "value"), &TaskGraphNode::_on_custom_param_changed);
}

void TaskGraphNode::_build_ui() {
    MemoryGraphNode::_build_ui(); // Generates ports and base states

    Dictionary props = get_properties();
    
    // Extract base identifiers injected by the GraphEdit Spawn Cache
    if (props.has("type_id")) task_type = static_cast<uint32_t>(props["type_id"]);
    if (props.has("logic_id")) logic_id = static_cast<uint32_t>(props["logic_id"]);
    if (props.has("logic_name")) logic_name = props["logic_name"];

    // 1. Header Setup
    task_type_label = memnew(Label);
    
    // Display the specific logic name rather than the generic Task Type
    String label_text = String("Logic: ") + logic_name;
    
    // Tint the node to visually separate Culler/Transform/Godot domains
    switch (task_type) {
        case TASK_GODOT_REFLECTION: set_self_modulate(Color(0.5f, 0.5f, 1.0f)); break; // Blue
        case TASK_NATIVE_CPU:       set_self_modulate(Color(1.0f, 0.5f, 0.5f)); break; // Red
        case TASK_COMPUTE_GPU:      set_self_modulate(Color(0.8f, 0.3f, 0.8f)); break; // Purple
        case TASK_QUERY_CULLER:     set_self_modulate(Color(0.3f, 0.8f, 0.3f)); break; // Green
        default: break;
    }
    
    task_type_label->set_text(label_text);
    add_child(task_type_label);

    // 2. Dynamic UI Container Setup
    custom_parameters_container = memnew(VBoxContainer);
    custom_parameters_container->set_name("CustomParameters");
    add_child(custom_parameters_container);

    // Sub-nodes override this to populate their specific OptionButtons/SpinBoxes
    _rebuild_dynamic_ui();
}

void TaskGraphNode::_notification(int p_what) {
    // CRITICAL: Call parent to ensure Layout Headers and Memory Telemetry badges are drawn
    MemoryGraphNode::_notification(p_what);

    if (p_what == NOTIFICATION_DRAW) {
        if (workspace_state != WORKSPACE_HIDDEN) {
            Ref<Texture2D> badge_icon = _get_badge_icon_for_workspace(workspace_state);
            if (badge_icon.is_valid()) {
                // Draw in the top-left corner (Inset slightly from the frame)
                // This keeps it opposite to the Tier 2 Memory Telemetry badge on the right
                Vector2 badge_pos = Vector2(10, 5);
                draw_texture(badge_icon, badge_pos);
            }
        }
    }
}

Ref<Texture2D> TaskGraphNode::_get_badge_icon_for_workspace(TransientWorkspaceState p_state) const {
    switch (p_state) {
        case WORKSPACE_ACTIVE: return get_theme_icon("badge_transient_active");
        case WORKSPACE_ERROR:  return get_theme_icon("badge_transient_error");
        case WORKSPACE_HIDDEN: 
        default:               return Ref<Texture2D>();
    }
}

void TaskGraphNode::set_workspace_state(TransientWorkspaceState p_state) {
    if (workspace_state == p_state) return;
    
    workspace_state = p_state;
    queue_redraw(); // Invoke NOTIFICATION_DRAW
}

void TaskGraphNode::_rebuild_dynamic_ui() {
    if (!custom_parameters_container) return;

    // Safely teardown existing dynamic controls before a logic switch or rebuild
    for (int i = 0; i < custom_parameters_container->get_child_count(); ++i) {
        Node* child = custom_parameters_container->get_child(i);
        child->queue_free();
    }
}

void TaskGraphNode::_update_matrix_guardrails() {
    // Base implementation is empty. Sub-nodes override this to parse the 
    // registry valid_combinations PackedInt64Array based on their dimensions.
}

uint64_t TaskGraphNode::_calculate_flat_index() const {
    return 0; // Base implementation
}

void TaskGraphNode::_on_custom_param_changed(const StringName& p_param_name, const Variant& p_value) {
    // 1. Immediately route the parameter change up to the graph resource
    emit_property_changed(p_param_name, p_value);

    // 2. Re-evaluate the matrix. 
    // If a View or Strategy dropdown changed, it might invalidate the other selections.
    _update_matrix_guardrails();
}

} // namespace ideam::godot_ext