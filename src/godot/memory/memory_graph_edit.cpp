#include "memory_graph_edit.h"
#include "memory_graph_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void MemoryGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_telemetry_mapping", "map"), &MemoryGraphEdit::set_telemetry_mapping);
    ClassDB::bind_method(D_METHOD("push_telemetry", "core_id", "inspector"), &MemoryGraphEdit::push_telemetry);
    
    ClassDB::bind_method(D_METHOD("_on_connection_to_empty", "from_node", "from_port", "release_position"), &MemoryGraphEdit::_on_connection_to_empty);
    ClassDB::bind_method(D_METHOD("_filtered_popup_select", "id"), &MemoryGraphEdit::_filtered_popup_select);
}

MemoryGraphEdit::MemoryGraphEdit() {
}

MemoryGraphEdit::~MemoryGraphEdit() {
}

void MemoryGraphEdit::_ready() {
    IdeamGraphEdit::_ready(); // Initialize base connections

    _create_filtered_popup();

    // Listen for user dragging a port connection into empty canvas space
    connect("connection_to_empty", Callable(this, "_on_connection_to_empty"));
}

void MemoryGraphEdit::_create_filtered_popup() {
    if (filtered_popup) return;
    
    filtered_popup = memnew(PopupMenu);
    add_child(filtered_popup);
    filtered_popup->connect("id_pressed", Callable(this, "_filtered_popup_select"));
}

void MemoryGraphEdit::_on_connection_to_empty(const StringName &p_from_node, int p_from_port, const Vector2 &p_release_position) {
    Node* ui_node = get_node_or_null(NodePath(p_from_node));
    MemoryGraphNode* m_node = Object::cast_to<MemoryGraphNode>(ui_node);
    
    uint32_t output_signature = 0; // TRAIT_NONE by default
    
    if (m_node) {
        // Query the specific traits of the port the user just dragged from
        output_signature = m_node->get_port_signature(p_from_port, true);
    }
    
    // Open the popup, passing the signature to filter the list
    _show_filtered_popup(p_release_position, output_signature);
}

void MemoryGraphEdit::_show_filtered_popup(const Vector2 &p_at, uint32_t p_filter_mask) {
    if (!filtered_popup) return;
    
    filtered_popup->clear();
    current_filter_mask = p_filter_mask;

    // Pass the filter mask down to the list generator
    TypedArray<String> types = _get_filtered_node_types(p_filter_mask);
    
    // If nothing matches, add a disabled hint to guide the user
    if (types.is_empty()) {
        filtered_popup->add_item("No compatible nodes found", 0);
        filtered_popup->set_item_disabled(0, true);
    } else {
        for (int i = 0; i < types.size(); ++i) {
            filtered_popup->add_item(types[i], i);
        }
    }
    
    memory_popup_position = p_at;
    filtered_popup->set_position(Vector2i(get_global_mouse_position().x, get_global_mouse_position().y));
    filtered_popup->popup();
}

void MemoryGraphEdit::_filtered_popup_select(int p_id) {
    // Route the creation natively through the Resource blueprint so Undo/Redo is respected
    Ref<ideam::godot_ext::IdeamGraphResource> blueprint = get_blueprint();
    if (blueprint.is_null()) return;

    StringName unique_name = String("MemNode_") + String::num_int64(UtilityFunctions::randi());

    Dictionary new_node;
    new_node["name"] = unique_name;
    new_node["type_id"] = p_id; 
    
    // Adjust for viewport scroll offset so the node spawns exactly where the wire dropped
    new_node["position"] = memory_popup_position + get_scroll_offset();
    
    blueprint->action_add_node(new_node);
}

TypedArray<String> MemoryGraphEdit::_get_filtered_node_types(uint32_t p_filter_mask) const {
    TypedArray<String> arr;
    
    // Example context-aware filtering logic:
    // If the user drags a spatial port wire, we only show spatial task nodes.
    if (p_filter_mask & TRAIT_SPATIAL_ACCESS) {
        arr.push_back("Spatial Blur Task");
        arr.push_back("Morphological Dilation Task");
    } else if (p_filter_mask & TRAIT_LINEAR_ACCESS) {
        arr.push_back("Linear Data Scatter Task");
        arr.push_back("Value Accumulation Task");
    } else {
        // Generic fallback if dragging from a non-strict port
        arr.push_back("Generic Memory Node");
        arr.push_back("Buffer Allocator");
    }
    
    return arr;
}

// --- Telemetry Routing Implementation ---

void MemoryGraphEdit::set_telemetry_mapping(const Dictionary& p_map) {
    dod_to_ui_map.clear();
    Array keys = p_map.keys();
    
    for (int i = 0; i < keys.size(); ++i) {
        Variant key = keys[i];
        Variant value = p_map[key];
        
        // Safely extract int keys and string names
        if (key.get_type() == Variant::INT && (value.get_type() == Variant::STRING || value.get_type() == Variant::STRING_NAME)) {
            uint32_t core_id = static_cast<uint32_t>(static_cast<int>(key));
            dod_to_ui_map[core_id] = value;
        }
    }
}

void MemoryGraphEdit::push_telemetry(int p_core_id, const Ref<ideam::godot_ext::MemoryGrantInspector>& p_inspector) {
    uint32_t core_id = static_cast<uint32_t>(p_core_id);
    
    // 1. O(1) Reverse Lookup: Find the Godot StringName for this physical ID
    auto it = dod_to_ui_map.find(core_id);
    if (it != dod_to_ui_map.end()) {
        
        // 2. Fetch the actual child UI Node from the GraphEdit canvas
        Node* child = get_node_or_null(NodePath(it->second));
        MemoryGraphNode* m_node = Object::cast_to<MemoryGraphNode>(child);
        
        // 3. Push the snapshot into the node for visual updates
        if (m_node) {
            m_node->update_telemetry(p_inspector);
        }
    }
}

} // namespace godot