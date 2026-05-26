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
    ClassDB::bind_method(D_METHOD("get_task_node_resource"), &TaskGraphNode::get_task_node_resource);
    ClassDB::bind_method(D_METHOD("get_task_type"), &TaskGraphNode::get_task_type);
    ClassDB::bind_method(D_METHOD("get_task_name"), &TaskGraphNode::get_task_name);
    
    ClassDB::bind_method(D_METHOD("_on_custom_param_changed", "param_name", "value"), &TaskGraphNode::_on_custom_param_changed);
    ClassDB::bind_method(D_METHOD("_on_buffer_option_selected", "index", "prop_name", "btn"), &TaskGraphNode::_on_buffer_option_selected);
}

Ref<TaskResource> TaskGraphNode::get_task_node_resource() const {
    return Object::cast_to<TaskResource>(get_node_resource().ptr());
}

uint32_t TaskGraphNode::get_task_type() const {
    Ref<TaskResource> res = get_task_node_resource();
    return res.is_valid() ? static_cast<uint32_t>(res->get_task_type()) : 0;
}

StringName TaskGraphNode::get_task_name() const {
    Ref<TaskResource> res = get_task_node_resource();
    return res.is_valid() ? res->get_task_name() : StringName();
}

void TaskGraphNode::_build_ui() {
    MemoryGraphNode::_build_ui(); // Generates ports and base states

    task_type_badge = memnew(godot::TextureRect);
    task_type_badge->set_name("TaskTypeBadge");
    add_badge(task_type_badge);

    workspace_badge = memnew(godot::TextureRect);
    workspace_badge->set_name("WorkspaceBadge");
    add_badge(workspace_badge);

    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_null()) return;

    // 1. Header Setup
    task_type_label = memnew(Label);
    String label_text = String("Logic: ") + get_task_name();
    task_type_label->set_text(label_text);
    add_child(task_type_label);

    // 2. Dynamic UI Container Setup
    custom_parameters_container = memnew(VBoxContainer);
    custom_parameters_container->set_name("CustomParameters");
    add_child(custom_parameters_container);

    // 3. Logic Inspector Setup (Persistent parameter controls)
    logic_inspector = memnew(RuntimeInspector);
    logic_inspector->set_name("LogicInspector");
    add_child(logic_inspector);
    
    // Route the inspector's mutations up to the graph resource
    logic_inspector->connect("property_changed", Callable(this, "_on_custom_param_changed"));

    _rebuild_dynamic_ui();
}

void TaskGraphNode::_notification(int p_what) {
    MemoryGraphNode::_notification(p_what);
}

void TaskGraphNode::_update_theme_properties() {
    // 1. Cascade down through structural parent layers (updates slots and metrics)
    MemoryGraphNode::_update_theme_properties();

    Ref<TaskResource> task_res = get_task_node_resource();
    StringName type_context = "GraphNode";

    // 2. Centralized color assignment pulling directly from active theme tokens
    if (workspace_badge) {
        workspace_badge->set_texture(_get_badge_icon_for_workspace(workspace_state));
        
        // Since this badge used to use a custom color modulation in the draw call:
        godot::Color badge_color = get_theme_color(workspace_state == WORKSPACE_ACTIVE ? "transient_active_color" : "transient_error_color", "GraphNode");
        workspace_badge->set_modulate(badge_color); 
        workspace_badge->set_visible(workspace_state != WORKSPACE_HIDDEN);
    }
    // 3. Propagate theme updates down to any dynamically instantiated buttons
    for (const auto& binding : buffer_option_bindings) {
        if (binding.button) {
            // If you have specific styles for dropdown inputs, apply them here.
            // Otherwise, they will naturally look up their custom Theme values from the control tree.
            binding.button->queue_redraw();
        }
    }

}

Ref<Texture2D> TaskGraphNode::_get_badge_icon_for_workspace(TransientWorkspaceState p_state) const {
    StringName type_context = "GraphNode";

    switch (p_state) {
        case WORKSPACE_ACTIVE: return get_theme_icon("badge_transient_active", type_context);
        case WORKSPACE_ERROR:  return get_theme_icon("badge_transient_error", type_context);
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

    // Also clear the logic inspector to prevent ghost parameters
    if (logic_inspector) {
        logic_inspector->clear_inspector();
    }
}

void TaskGraphNode::_reify_property_schema(Array& r_properties, uint32_t p_current_type_id) {
    buffer_option_bindings.clear();

    godot::Ref<TaskResource> task_res = get_task_node_resource();
    godot::Ref<MemoryGrantResource> grant_res;
    
    if (task_res.is_valid()) {
        grant_res = task_res->get_memory_grant();
    }

    for (int i = 0; i < r_properties.size(); ++i) {
        Dictionary prop = r_properties[i];

        // 1. Resolve dynamic 'T' types
        if (prop.has("type") && prop["type"].get_type() == Variant::STRING && String(prop["type"]) == "T") {
            if (prop.has("type_hints")) {
                Dictionary hints = prop["type_hints"];
                
                if (hints.has(p_current_type_id)) {
                    Dictionary concrete_hint = hints[p_current_type_id];
                    
                    // Overwrite generic markers with concrete variant instructions
                    prop["type"] = concrete_hint.has("type") ? concrete_hint["type"] : Variant::NIL;
                    if (concrete_hint.has("hint")) prop["hint"] = concrete_hint["hint"];
                    if (concrete_hint.has("hint_string")) prop["hint_string"] = concrete_hint["hint_string"];
                } else {
                    // Fallback if the selected DOD type isn't mapped in the logic struct
                    prop["type"] = Variant::NIL; 
                }
                
                // Erase the hints payload to save memory overhead in the UI layer
                prop.erase("type_hints");
            }
        }

        // 2. Recursively resolve nested structs (e.g., Array of GroupMaskMapping)
        if (prop.has("struct_properties")) {
            Array sub_schema = prop["struct_properties"];
            // Recurse down into the struct's definition
            _reify_property_schema(sub_schema, p_current_type_id);
            prop["struct_properties"] = sub_schema;
        }
        if (prop.has("port_override")) {
            uint64_t mask = prop["port_override"];
            
            uint8_t slot          = mask & 0xFF;
            bool override_left    = (mask >> 8) & 1;
            bool enable_left      = (mask >> 9) & 1;
            auto layout_left      = static_cast<core::BufferLayoutType>((mask >> 10) & 0xFFFF);
            
            bool override_right   = (mask >> 26) & 1;
            bool enable_right     = (mask >> 27) & 1;
            auto layout_right     = static_cast<core::BufferLayoutType>((mask >> 28) & 0xFFFF);

            // Execute the UI update. 
            // update_memory_port() naturally sets icons based on the BufferLayoutType.
            if (override_left) {
                set_slot_enabled_left(slot, enable_left);
                if (enable_left) update_memory_port(slot, true, godot::BitField<core::BufferLayoutType>(static_cast<int64_t>(layout_left)));
            }
            
            if (override_right) {
                set_slot_enabled_right(slot, enable_right);
                if (enable_right) update_memory_port(slot, false, godot::BitField<core::BufferLayoutType>(static_cast<int64_t>(layout_right)));
            }
        }
        // Write the mutated dictionary back to the array. 
        // (Required because extracting `prop` creates a shallow Variant copy of the dictionary ref)
        r_properties[i] = prop;

        if (static_cast<int>(prop["type"]) == godot::Variant::INT && godot::String(prop["hint_string"]) == "buffer_option") {
            godot::StringName prop_name = prop["name"];
            
            godot::OptionButton* opt_btn = memnew(godot::OptionButton);
            custom_parameters_container->add_child(opt_btn);
            
            BufferOptionBinding binding;
            binding.property_name = prop_name;
            binding.button = opt_btn;

            // Extract valid layout dimensions from the Grant Resource
            if (grant_res.is_valid()) {
                godot::TypedArray<GrantPartResource> parts = grant_res->get_configured_parts();
                binding.buffer_ids.resize(parts.size());
                
                for (int j = 0; j < parts.size(); ++j) {
                    godot::Ref<GrantPartResource> part = parts[j];
                    if (part.is_valid()) {
                        binding.buffer_ids.set(j, part->get_buffer_id());
                    }
                }
            }

            // Bind the signal with payload injection (prop_name and the button pointer)
            opt_btn->connect("item_selected", godot::Callable(this, "_on_buffer_option_selected").bind(prop_name, opt_btn));

            buffer_option_bindings.push_back(binding);
            
            // Remove the property from the schema so the RuntimeInspector ignores it
            r_properties.remove_at(i);
        }   
    }

    // If we registered any async dropdowns, request the current topological names
    if (!buffer_option_bindings.empty()) {
        emit_signal("buffer_names_requested");
    }
}

void TaskGraphNode::_rebuild_logic_inspector(const Array& p_properties) {
    if (!logic_inspector) return;
    
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_null()) return;

    Dictionary state = task_res->get_task_properties();
    
    // We must resolve what the generic type "T" represents for DOD alignments
    Variant::Type resolved_t = Variant::NIL;
    uint32_t current_type_id = 0; // Default fallback
    
    if (state.has("type_id")) {
        //current_type_id = task_res->get_type_id();
        current_type_id = static_cast<uint32_t>(state["type_id"]);
        resolved_t = static_cast<Variant::Type>(current_type_id);
    }
    
    // CRITICAL: Deep copy the registry blueprint so we don't permanently overwrite 
    // the generic "T" markers for other nodes using this same logic ID.
    Array instanced_schema = p_properties.duplicate(true);
    
    // Perform the Type Reification pass
    _reify_property_schema(instanced_schema, current_type_id);
    
    // Hand the concrete, flattened schema to the inspector
    logic_inspector->build_inspector(instanced_schema, state, resolved_t);
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
    // IdeamGraphEdit will catch this and update the resource's dictionary
    emit_property_changed(p_param_name, p_value);

    // 2. Re-evaluate the matrix. 
    // If a View or Strategy dropdown changed, it might invalidate the other selections.
    _update_matrix_guardrails();
}

void TaskGraphNode::_on_buffer_option_selected(int p_index, godot::StringName p_prop_name, godot::OptionButton* p_btn) {
    if (!p_btn) return;
    
    // The true buffer ID is stored in the OptionButton's item metadata/ID
    int selected_buffer_id = p_btn->get_item_id(p_index);
    
    // Route to GraphEdit to mutate the TaskResource dictionary
    emit_property_changed(p_prop_name, selected_buffer_id);
}

void TaskGraphNode::receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) {
    // Fulfill any base MemoryGraphNode logic first
    MemoryGraphNode::receive_buffer_names_list(p_names); 

    // Rapid iteration over cached UI bindings
    for (const auto& binding : buffer_option_bindings) {
        godot::OptionButton* btn = binding.button;
        if (!btn) continue;

        // Cache the currently selected ID to restore it after population
        int current_selected_id = btn->get_selected_id();
        
        btn->clear();

        for (int i = 0; i < binding.buffer_ids.size(); ++i) {
            int b_id = binding.buffer_ids[i];
            godot::String b_name = "Unknown Buffer";
            
            // Validate the ID against the provided topological name list
            if (b_id >= 0 && b_id < p_names.size()) {
                b_name = p_names[b_id];
            }
            
            btn->add_item(b_name, b_id);
        }

        // Restore selection safely
        int new_idx = btn->get_item_index(current_selected_id);
        if (new_idx != -1) {
            btn->select(new_idx);
        } else if (btn->get_item_count() > 0) {
            // Default to index 0 if the previous selection became invalid
            btn->select(0);
        }
    }
}

} // namespace ideam::godot_ext