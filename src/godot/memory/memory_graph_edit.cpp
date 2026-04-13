#include "memory_graph_edit.h"
#include "memory_graph_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

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
    filtered_popup = memnew(PopupMenu);
    filtered_popup->set_name("FilteredContextPopup");
    add_child(filtered_popup);
    filtered_popup->connect("id_pressed", Callable(this, "_filtered_popup_select"));
}

void MemoryGraphEdit::_on_connection_to_empty(const StringName &p_from_node, int p_from_port, const Vector2 &p_release_position) {
    // 1. Resolve the node being dragged from
    Node* raw_node = get_node_or_null(NodePath(p_from_node));
    MemoryGraphNode* mem_node = Object::cast_to<MemoryGraphNode>(raw_node);
    
    if (!mem_node) return;

    // 2. Query its signature for that specific output port
    current_filter_mask = mem_node->get_port_signature(p_from_port, true);

    // 3. Display the tailored popup
    _show_filtered_popup(p_release_position, current_filter_mask);
}

void MemoryGraphEdit::_show_filtered_popup(const Vector2 &p_at, uint32_t p_filter_mask) {
    filtered_popup->clear();
    
    TypedArray<String> valid_nodes = _get_filtered_node_types(p_filter_mask);
    
    if (valid_nodes.is_empty()) {
        filtered_popup->add_item("No compatible nodes found", -1);
        filtered_popup->set_item_disabled(0, true);
    } else {
        for (int i = 0; i < valid_nodes.size(); ++i) {
            filtered_popup->add_item(valid_nodes[i], i); // Use index as ID for now
        }
    }

    memory_popup_position = p_at;
    
    // Convert local position to screen position for the popup
    Vector2 screen_pos = get_screen_transform().xform(p_at);
    filtered_popup->set_position(screen_pos);
    filtered_popup->popup();
}

void MemoryGraphEdit::_filtered_popup_select(int p_id) {
    if (p_id < 0) return; // Disabled/empty item clicked

    // Route back through the base class spawning mechanism if possible,
    // or implement specialized spawning here.
    // For now, delegate to base class logic (assuming ID mapping holds)
    _spawn_node_by_type(p_id); 
}

TypedArray<String> MemoryGraphEdit::_get_filtered_node_types(uint32_t p_filter_mask) const {
    TypedArray<String> arr;
    
    // In a full implementation, we'd iterate the registry.
    // Dummy implementation for structure:
    if (p_filter_mask & TRAIT_LINEAR_ACCESS) {
        arr.push_back("Flat Buffer Allocator");
    }
    
    if (p_filter_mask & TRAIT_SPATIAL_ACCESS) {
        arr.push_back("Spatial Partition Buffer");
        arr.push_back("Voxel Grid Allocator");
    }
    
    if (p_filter_mask == TRAIT_NONE) {
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

void MemoryGraphEdit::push_telemetry(int p_core_id, const Ref<MemoryGrantInspector>& p_inspector) {
    uint32_t core_id = static_cast<uint32_t>(p_core_id);
    
    // 1. O(1) Reverse Lookup: Find the Godot StringName for this physical ID
    auto it = dod_to_ui_map.find(core_id);
    if (it != dod_to_ui_map.end()) {
        StringName ui_name = it->second;
        
        // 2. Fetch the UI node directly
        Node* raw_node = get_node_or_null(NodePath(ui_name));
        MemoryGraphNode* mem_node = Object::cast_to<MemoryGraphNode>(raw_node);
        
        if (mem_node) {
            mem_node->update_telemetry(p_inspector);
        }
    }
}

} // namespace ideam::godot_ext