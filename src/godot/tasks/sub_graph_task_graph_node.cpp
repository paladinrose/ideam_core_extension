#include "sub_graph_task_graph_node.h"
#include "task_graph_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

SubGraphTaskGraphNode::SubGraphTaskGraphNode() {}

void SubGraphTaskGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_add_mapping_pressed"), &SubGraphTaskGraphNode::_on_add_mapping_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_mapping_pressed", "row_node"), &SubGraphTaskGraphNode::_on_remove_mapping_pressed);
    ClassDB::bind_method(D_METHOD("_on_mapping_changed", "index"), &SubGraphTaskGraphNode::_on_mapping_changed);
}

void SubGraphTaskGraphNode::_rebuild_dynamic_ui() {
    TaskGraphNode::_rebuild_dynamic_ui();
    if (!custom_parameters_container) return;

    // Wipe cached state in case this is a full rebuild
    available_buffer_ids.clear();
    available_buffer_names.clear();

    // 1. Static Layout Base
    child_graph_btn = memnew(Button);
    child_graph_btn->set_text("< Select Child Graph Resource >");
    custom_parameters_container->add_child(child_graph_btn);

    mappings_foldable = memnew(FoldableContainer);
    mappings_foldable->set_title("Grant Mappings"); 
    custom_parameters_container->add_child(mappings_foldable);

    ScrollContainer* scroll_container = memnew(ScrollContainer);
    scroll_container->set_custom_minimum_size(Vector2(0, 150)); 
    mappings_foldable->add_child(scroll_container);

    mappings_list_container = memnew(VBoxContainer);
    mappings_list_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scroll_container->add_child(mappings_list_container);

    Button* add_btn = memnew(Button);
    add_btn->set_text("+ New Grant Mapping");
    add_btn->connect("pressed", callable_mp(this, &SubGraphTaskGraphNode::_on_add_mapping_pressed));
    custom_parameters_container->add_child(add_btn);

    // 2. Extract strictly permitted buffer IDs from the memory grant
    Ref<SubGraphTaskResource> res = Object::cast_to<SubGraphTaskResource>(get_task_node_resource().ptr());
    if (res.is_valid()) {
        Ref<MemoryGrantResource> grant = res->get_memory_grant();
        if (grant.is_valid()) {
            available_buffer_ids = grant->get_buffer_ids();
        }
    }

    // 3. Fire request to the central registry. 
    // We pass the explicit Array of IDs we are authorized for.
    Array request_array;
    for (int i = 0; i < available_buffer_ids.size(); ++i) {
        request_array.push_back(available_buffer_ids[i]);
    }
    
    emit_signal("buffer_names_requested", this, request_array);
    // Note: We stop here. _add_mapping_row calls are deferred to receive_buffer_names_list.
}

void SubGraphTaskGraphNode::receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) {
    TaskGraphNode::receive_buffer_names_list(p_names);
    available_buffer_names = p_names;
    available_child_nodes.clear();

    Ref<SubGraphTaskResource> res = Object::cast_to<SubGraphTaskResource>(get_task_node_resource().ptr());
    if (!res.is_valid()) return;

    // --- Fetch Child Graph Entry Wave ---
    Ref<TaskGraphResource> child_res = res->get_child_graph();
    if (child_res.is_valid()) {
        TypedArray<TypedArray<StringName>> waves = child_res->get_execution_waves();
        if (!waves.is_empty()) {
            available_child_nodes = waves[0];
        }
    }

    // --- Populate the visual rows from the StringName Dictionary ---
    Dictionary mappings = res->get_grant_mappings();
    Array keys = mappings.keys();
    for (int i = 0; i < keys.size(); ++i) {
        int parent_id = keys[i];
        StringName child_name = mappings[keys[i]];
        _add_mapping_row(parent_id, child_name);
    }
}

void SubGraphTaskGraphNode::_add_mapping_row(int p_parent_id, const godot::StringName& p_child_node_name) {
    if (!mappings_list_container) return;

    HBoxContainer* row = memnew(HBoxContainer);
    mappings_list_container->add_child(row);

    // --- Parent Buffer Dropdown ---
    OptionButton* parent_opt = memnew(OptionButton);
    parent_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    row->add_child(parent_opt);
    
    int selected_parent_idx = -1;
    for (int i = 0; i < available_buffer_ids.size(); ++i) {
        int b_id = available_buffer_ids[i];
        String b_name = (i < available_buffer_names.size()) ? String(available_buffer_names[i]) : "Invalid Buffer";
        
        parent_opt->add_item(b_name, b_id);
        if (b_id == p_parent_id) selected_parent_idx = i;
    }

    if (selected_parent_idx != -1) parent_opt->select(selected_parent_idx);
    else if (parent_opt->get_item_count() > 0) parent_opt->select(0); 

    parent_opt->connect("item_selected", callable_mp(this, &SubGraphTaskGraphNode::_on_mapping_changed));

    // --- Child Node Dropdown ---
    OptionButton* child_opt = memnew(OptionButton);
    child_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    row->add_child(child_opt);
    
    int selected_child_idx = -1;
    for (int i = 0; i < available_child_nodes.size(); ++i) {
        StringName c_name = available_child_nodes[i];
        child_opt->add_item(c_name, i);
        // CRITICAL: We bind the StringName directly to the item metadata so we can extract it cleanly later
        child_opt->set_item_metadata(i, c_name); 
        
        if (c_name == p_child_node_name) selected_child_idx = i;
    }

    if (selected_child_idx != -1) child_opt->select(selected_child_idx);
    else if (child_opt->get_item_count() > 0) child_opt->select(0);

    child_opt->connect("item_selected", callable_mp(this, &SubGraphTaskGraphNode::_on_mapping_changed));

    // --- Size Separator & Destruction ---
    Control* spacer = memnew(Control);
    spacer->set_custom_minimum_size(Vector2(10, 0));
    row->add_child(spacer);

    Button* remove_btn = memnew(Button);
    remove_btn->set_text("X");
    remove_btn->connect("pressed", callable_mp(this, &SubGraphTaskGraphNode::_on_remove_mapping_pressed).bind(row));
    row->add_child(remove_btn);
}

void SubGraphTaskGraphNode::_on_add_mapping_pressed() {
    int default_parent_id = available_buffer_ids.is_empty() ? 0 : available_buffer_ids[0];
    godot::StringName default_child = available_child_nodes.is_empty() ? godot::StringName("") : static_cast<godot::StringName>(available_child_nodes[0]);
    
    _add_mapping_row(default_parent_id, default_child);
    _sync_mappings_to_resource();
}

void SubGraphTaskGraphNode::_on_remove_mapping_pressed(godot::Node* p_row) {
    if (!p_row || !mappings_list_container) return;
    p_row->queue_free();
    callable_mp(this, &SubGraphTaskGraphNode::_sync_mappings_to_resource).call_deferred();
}

void SubGraphTaskGraphNode::_on_mapping_changed(int p_index) {
    _sync_mappings_to_resource();
}

void SubGraphTaskGraphNode::_sync_mappings_to_resource() {
    if (!mappings_list_container) return;

    // Use a Variant Dictionary to hold the [ParentBufferID : ChildNodeName] mappings
    godot::Dictionary mappings;

    for (int i = 0; i < mappings_list_container->get_child_count(); ++i) {
        HBoxContainer* row = Object::cast_to<HBoxContainer>(mappings_list_container->get_child(i));
        if (!row || row->is_queued_for_deletion()) continue;

        OptionButton* parent_opt = Object::cast_to<OptionButton>(row->get_child(0));
        OptionButton* child_opt = Object::cast_to<OptionButton>(row->get_child(1));

        if (parent_opt && child_opt) {
            int parent_id = parent_opt->get_selected_id();
            int child_idx = child_opt->get_selected();
            
            if (parent_id != -1 && child_idx != -1) {
                godot::StringName child_name = child_opt->get_item_metadata(child_idx);
                mappings[parent_id] = child_name;
            }
        }
    }

    _on_custom_param_changed("grant_mappings", mappings);
}

} // namespace ideam::godot_ext