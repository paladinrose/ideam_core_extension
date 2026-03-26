#include "task_graph_node.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void TaskGraphNode::_bind_methods() {
    // Methods for synchronization can be exposed if needed for GDScript interop
}

TaskGraphNode::TaskGraphNode() {
    main_container = memnew(VBoxContainer);
    add_child(main_container);

    input_rows = memnew(VBoxContainer);
    main_container->add_child(input_rows);

    output_rows = memnew(VBoxContainer);
    main_container->add_child(output_rows);
}

TaskGraphNode::~TaskGraphNode() {
}

void TaskGraphNode::_ready() {
    IdeamGraphNode::_ready();
}

void TaskGraphNode::set_graph_context(ideam::core::TaskGraph* p_graph) {
    task_graph_ptr = p_graph;
}

void TaskGraphNode::sync_with_core() {
    if (!task_graph_ptr) return;

    ideam::core::TaskMetadata* metadata = task_graph_ptr->get_task(get_core_node_id());
    if (!metadata) return;

    // 1. Update Identity
    set_title(metadata->title.c_str());
    
    // 2. Clear existing ports
    for (int i = 0; i < get_child_count(); ++i) {
        // Implementation detail: GraphEdit manages slot indices. 
        // We ensure rows are managed within our sub-containers.
    }

    // 3. Build Input Ports
    int slot_idx = 0;
    for (const auto& mapping : metadata->input_mappings) {
        Label* lbl = memnew(Label);
        lbl->set_text(mapping.internal_key.c_str());
        input_rows->add_child(lbl);
        
        // Slot logic: Left side, type-specific color
        set_slot(slot_idx, true, 0, Color(1,1,1), false, 0, Color(1,1,1)); 
        slot_idx++;
    }

    // 4. Build Output Ports
    for (const auto& mapping : metadata->output_mappings) {
        Label* lbl = memnew(Label);
        lbl->set_text(mapping.internal_key.c_str());
        lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        output_rows->add_child(lbl);
        
        // Slot logic: Right side
        set_slot(slot_idx, false, 0, Color(1,1,1), true, 0, Color(1,1,1));
        slot_idx++;
    }

    update_status_visuals();
}

void TaskGraphNode::update_status_visuals() {
    if (!task_graph_ptr) return;
    ideam::core::TaskMetadata* metadata = task_graph_ptr->get_task(get_core_node_id());
    if (!metadata) return;

    switch (metadata->status) {
        case ideam::core::TaskStatus::PLANNING:
            set_self_modulate(COLOR_PLANNING);
            break;
        case ideam::core::TaskStatus::IN_PROGRESS:
            set_self_modulate(COLOR_IN_PROGRESS);
            break;
        case ideam::core::TaskStatus::COMPLETE:
            set_self_modulate(COLOR_COMPLETE);
            break;
        case ideam::core::TaskStatus::FAILED:
            set_self_modulate(COLOR_FAILED);
            break;
        default:
            break;
    }
}

Color TaskGraphNode::get_color_for_type(ideam::core::DataType p_type) {
    switch (p_type) {
        case ideam::core::DataType::FLOAT32: return Color(0.3, 0.7, 1.0); // Light Blue
        case ideam::core::DataType::INT32:   return Color(0.3, 1.0, 0.5); // Green
        case ideam::core::DataType::VECTOR3: return Color(1.0, 0.8, 0.2); // Yellow/Gold
        case ideam::core::DataType::COLOR:   return Color(1.0, 0.4, 0.8); // Pink
        default: return Color(1, 1, 1);
    }
}

TypedArray<String> TaskGraphNode::get_context_menu_options() const {
    TypedArray<String> options;
    options.push_back("Set as Entry Point");
    options.push_back("Set as Exit Point");
    options.push_back("---");
    options.push_back("Reset Task");
    options.push_back("Delete");
    return options;
}

void TaskGraphNode::select_context_menu_option(int p_id) {
    switch (p_id) {
        case 0: // Entry Point logic
            break;
        case 3: // Reset
            if (task_graph_ptr) {
                ideam::core::TaskMetadata* m = task_graph_ptr->get_task(get_core_node_id());
                if (m) m->status = ideam::core::TaskStatus::PLANNING;
                update_status_visuals();
            }
            break;
    }
}

} // namespace godot