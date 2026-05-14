#include "entry_fill_task_graph_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/label.hpp>

using namespace godot;

namespace ideam::godot_ext {

void EntryFillTaskGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_buffer_selected", "index"), &EntryFillTaskGraphNode::_on_buffer_selected);
}

EntryFillTaskGraphNode::EntryFillTaskGraphNode() {
}

void EntryFillTaskGraphNode::_rebuild_dynamic_ui() {
    // 1. Execute base teardown and layout allocation
    TaskGraphNode::_rebuild_dynamic_ui();

    if (!custom_parameters_container) return;

    // 2. Add visual label for the user
    Label* label = memnew(Label);
    label->set_text("Target Buffer:");
    custom_parameters_container->add_child(label);

    // 3. Initialize the buffer selection dropdown
    buffer_dropdown = memnew(OptionButton);
    custom_parameters_container->add_child(buffer_dropdown);

    buffer_dropdown->connect("item_selected", callable_mp(this, &EntryFillTaskGraphNode::_on_buffer_selected));

    // 4. Fire the data request signal up to the controller (TaskGraphEdit / MemoryGraphEdit).
    // Supplying an empty Array() signals that we want an unfiltered list of ALL buffers.
    emit_signal("buffer_names_requested", this, Array());
}

void EntryFillTaskGraphNode::_on_buffer_selected(int p_index) {
    if (!buffer_dropdown) return;

    Ref<EntryFillTaskResource> res = Object::cast_to<EntryFillTaskResource>(get_task_node_resource().ptr());
    if (res.is_valid()) {
        // Because the architecture guarantees a 1:1 mapping between the buffer's 
        // sequential list order and its ID, the UI item index IS the actual buffer ID.
        res->set_target_buffer_id(p_index);
        
        // Notify the graph environment of the structural state change
        _on_custom_param_changed("target_buffer_id", p_index);
    }
}

void EntryFillTaskGraphNode::receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) {
    if (!buffer_dropdown) return;

    buffer_dropdown->clear();
    
    Ref<EntryFillTaskResource> res = Object::cast_to<EntryFillTaskResource>(get_task_node_resource().ptr());
    int current_target = res.is_valid() ? res->get_target_buffer_id() : -1;

    // Repopulate the list with the fresh memory topography
    for (int i = 0; i < p_names.size(); ++i) {
        // Godot's OptionButton implicitly assigns the linear index `i` as the item's ID,
        // natively keeping the UI synchronized with your DOD architecture.
        buffer_dropdown->add_item(String(p_names[i]));
    }

    // Attempt to restore the previously configured state
    if (current_target >= 0 && current_target < buffer_dropdown->get_item_count()) {
        buffer_dropdown->select(current_target);
    } 
    // Fallback: If the old target buffer was deleted from the architecture, default to 0
    // and forcefully update the backing Resource to keep the data state valid.
    else if (buffer_dropdown->get_item_count() > 0) {
        buffer_dropdown->select(0);
        _on_buffer_selected(0);
    }
}

} // namespace ideam::godot_ext