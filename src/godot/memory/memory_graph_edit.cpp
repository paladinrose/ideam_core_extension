#include "memory_graph_edit.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/font.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_telemetry_mapping", "map"), &MemoryGraphEdit::set_telemetry_mapping);
    ClassDB::bind_method(D_METHOD("push_telemetry", "core_id", "inspector"), &MemoryGraphEdit::push_telemetry);
    
    ClassDB::bind_method(D_METHOD("_on_buffer_names_requested", "node", "buffer_ids"), &MemoryGraphEdit::_on_buffer_names_requested);
    ClassDB::bind_method(D_METHOD("_memory_request_connect", "from_node", "from_port", "to_node", "to_port"), &MemoryGraphEdit::_memory_request_connect);
    ClassDB::bind_method(D_METHOD("_on_connection_to_empty", "from_node", "from_port", "release_position"), &MemoryGraphEdit::_on_connection_to_empty);
    ClassDB::bind_method(D_METHOD("_on_node_memory_grant_requested", "node"), &MemoryGraphEdit::_on_node_memory_grant_requested);
    ClassDB::bind_method(D_METHOD("_on_grant_window_payload_submitted", "node_name", "buffer_ids"), &MemoryGraphEdit::_on_grant_window_payload_submitted);
    
}

MemoryGraphEdit::MemoryGraphEdit() {
}

MemoryGraphEdit::~MemoryGraphEdit() {
}

void MemoryGraphEdit::_ready() {
    IdeamGraphEdit::_ready(); // Initialize base connections

    // Re-route the standard connection request to our DOD memory validator
    if (is_connected("connection_request", Callable(this, "_request_connect"))) {
        disconnect("connection_request", Callable(this, "_request_connect"));
    }
    connect("connection_request", Callable(this, "_memory_request_connect"));

    if (is_connected("connection_to_empty", Callable(this, "_on_connection_to_empty"))) {
        disconnect("connection_to_empty", Callable(this, "_on_connection_to_empty"));
    }
    connect("connection_to_empty", Callable(this, "_on_connection_to_empty"));

}

void MemoryGraphEdit::_notification(int p_what) {
    // Forward structural notifications to the parent class first
    IdeamGraphEdit::_notification(p_what);

    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme_properties();
        } break;
        case NOTIFICATION_DRAW: {
            _draw_custom_edges();
        } break;
    }
}

void MemoryGraphEdit::_update_theme_properties() {
    // 1. Invoke the base implementation to configure context_popup
    IdeamGraphEdit::_update_theme_properties();

    // 2. Synchronize our specialized sub-popup with the current PopupMenu style profiles
    if (filtered_popup) {
        Ref<StyleBox> panel_style = get_theme_stylebox("popup_menu_panel", "PopupMenu");
        if (panel_style.is_valid()) {
            filtered_popup->add_theme_stylebox_override("panel", panel_style);
        }
        
        Ref<StyleBox> hover_style = get_theme_stylebox("popup_menu_hover", "PopupMenu");
        if (hover_style.is_valid()) {
            filtered_popup->add_theme_stylebox_override("hover", hover_style);
        }
    }

    // 3. Clear layouts and force structural edge redraws
    _refresh_edge_cache();
    queue_redraw();
}

void MemoryGraphEdit::_on_buffer_names_requested(godot::Object* p_node, const godot::Array& p_buffer_ids) {
    MemoryGraphNode* requesting_node = Object::cast_to<MemoryGraphNode>(p_node);
    if (!requesting_node) return;

    // Cast the base IdeamGraphResource up to our memory-aware blueprint
    Ref<MemoryGraphResource> mem_blueprint = current_blueprint;
    if (mem_blueprint.is_null()) return;

    // Retrieve the authoritative DOD Manager Resource
    Ref<MemoryManagerResource> manager = mem_blueprint->get_memory_manager();
    if (manager.is_null()) return;

    TypedArray<StringName> names;

    // Route based on payload density
    if (p_buffer_ids.is_empty()) {
        names = manager->get_buffer_names();
    } else {
        // Unpack the Variant Array into a contiguous stack array for the resource fetcher
        PackedInt32Array packed_ids;
        packed_ids.resize(p_buffer_ids.size());
        for (int i = 0; i < p_buffer_ids.size(); ++i) {
            packed_ids.set(i, p_buffer_ids[i]);
        }
        names = manager->get_selected_buffer_names(packed_ids);
    }

    // Pass the contiguous string pointers back down to the UI node
    requesting_node->receive_buffer_names_list(names);
}

// ==========================================
// TIER 2: TOPOLOGICAL VALIDATION
// ==========================================

void MemoryGraphEdit::_memory_request_connect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;

    // 1. Phase 1 Verification (Locks)
    Node* from_n = get_node_or_null(NodePath(p_from_node));
    Node* to_n = get_node_or_null(NodePath(p_to_node));
    MemoryGraphNode* from_ign = Object::cast_to<MemoryGraphNode>(from_n);
    MemoryGraphNode* to_ign = Object::cast_to<MemoryGraphNode>(to_n);

    // FIX: Using the newly implemented strict $O(1)$ accessors from Phase 1, replacing Variant dictionary lookups.
    if ((from_ign && from_ign->get_locked()) || (to_ign && to_ign->get_locked())) {
        return; 
    }

    // 2. Determine Incoming Access Mode
    // Note: In full execution, this traverses the blueprint's task logic. For UI composition, 
    // we query the declared SFINAE signatures compiled into the UI port traits.
    core::BufferAccessMode incoming_mode = core::BufferAccessMode::READ;
    if (to_ign && (to_ign->get_port_signature(p_to_port, false) & TRAIT_LINEAR_ACCESS)) { // Simplified simulation flag for write intent
        incoming_mode = core::BufferAccessMode::WRITE; 
    }

    // 3. Collision Detection
    if (!_validate_write_collision(p_to_node, p_to_port, incoming_mode)) {
        if (from_ign) from_ign->update_port_state(p_from_port, false, IdeamGraphNode::PORT_ERROR);
        if (to_ign) to_ign->update_port_state(p_to_port, true, IdeamGraphNode::PORT_ERROR);
        UtilityFunctions::print_rich("[color=red]Memory Error: Mutative Write Collision detected. Requires ATOMIC declaration.[/color]");
        return; 
    }

    // 4. Fork Grant Rules
    if (!_validate_fork_grant(p_from_node, p_from_port, p_to_node, p_to_port)) {
        if (from_ign) from_ign->update_port_state(p_from_port, false, IdeamGraphNode::PORT_ERROR);
        if (to_ign) to_ign->update_port_state(p_to_port, true, IdeamGraphNode::PORT_ERROR);
        UtilityFunctions::print_rich("[color=red]Memory Error: Illegal Fork Grant. Cannot map SPARSE subset to RANGE requirement without gather task.[/color]");
        return;
    }

    // Validation passed, defer mutation to the DOD Resource
    Dictionary edge;
    edge["from"] = p_from_node;
    edge["from_port"] = p_from_port;
    edge["to"] = p_to_node;
    edge["to_port"] = p_to_port;
    
    // Note: Passing the resolved metadata to the resource for the draw cache to pick up later
    edge["access_mode"] = static_cast<int>(incoming_mode);

    current_blueprint->action_add_edge(edge);
    
    if (from_ign) _on_node_connections_requested(from_ign);
    if (to_ign) _on_node_connections_requested(to_ign);
    
    _refresh_edge_cache();
    queue_redraw();
}

bool MemoryGraphEdit::_validate_write_collision(const StringName &p_to_node, int p_to_port, core::BufferAccessMode p_incoming_mode) {
    if (p_incoming_mode != core::BufferAccessMode::WRITE && p_incoming_mode != core::BufferAccessMode::READ_WRITE) {
        return true; // Concurrent reads are perfectly safe in DOD
    }

    TypedArray<Dictionary> edges = current_blueprint->get_edges();
    for (int i = 0; i < edges.size(); ++i) {
        Dictionary e = edges[i];
        if (e["to"] == Variant(p_to_node) && static_cast<int>(e["to_port"]) == p_to_port) {
            core::BufferAccessMode existing_mode = static_cast<core::BufferAccessMode>(static_cast<int>(e.get("access_mode", 0)));
            
            if (existing_mode == core::BufferAccessMode::WRITE || p_incoming_mode == core::BufferAccessMode::WRITE) {
                // If ANY node writing to this column isn't explicitly atomic, block the connection.
                if (existing_mode != core::BufferAccessMode::READ_WRITE || p_incoming_mode != core::BufferAccessMode::READ_WRITE) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool MemoryGraphEdit::_validate_fork_grant(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    MemoryGraphNode* from_n = Object::cast_to<MemoryGraphNode>(get_node_or_null(NodePath(p_from_node)));
    MemoryGraphNode* to_n = Object::cast_to<MemoryGraphNode>(get_node_or_null(NodePath(p_to_node)));
    if (!from_n || !to_n) return true;

    uint32_t out_sig = from_n->get_port_signature(p_from_port, true);
    uint32_t in_sig = to_n->get_port_signature(p_to_port, false);
    
    // Abstract representation mapping: Hard reject a RANDOM (Sparse) trying to operate directly on a RANGE (Contiguous)
    if ((out_sig & TRAIT_RANDOM_ACCESS) && !(in_sig & TRAIT_RANDOM_ACCESS) && (in_sig & TRAIT_LINEAR_ACCESS)) {
         return false; 
    }
    return true;
}

// ==========================================
// TIER 2: EDGE TOPOGRAPHY (VISUALS)
// ==========================================

uint64_t MemoryGraphEdit::_hash_edge(const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port) const {
    return p_from.hash() ^ (p_from_port << 8) ^ (p_to.hash() << 16) ^ (p_to_port << 24);
}

void MemoryGraphEdit::_refresh_edge_cache() {
    if (current_blueprint.is_null()) return;
    
    edge_meta_cache.clear();
    TypedArray<Dictionary> edges = current_blueprint->get_edges();
    for (int i = 0; i < edges.size(); ++i) {
        Dictionary e = edges[i];
        uint64_t h = _hash_edge(e["from"], e["from_port"], e["to"], e["to_port"]);
        
        EdgeMetadata meta;
        meta.access_mode = static_cast<core::BufferAccessMode>(static_cast<int>(e.get("access_mode", 0)));
        meta.is_atomic = (meta.access_mode == core::BufferAccessMode::READ_WRITE);
        meta.is_error = static_cast<bool>(e.get("is_error", false));
        
        edge_meta_cache[h] = meta;
    }
}

Vector2 MemoryGraphEdit::_evaluate_bezier(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float t) const {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Vector2 p = uuu * p0; 
    p += 3 * uu * t * p1; 
    p += 3 * u * tt * p2; 
    p += ttt * p3;        
    return p;
}

void MemoryGraphEdit::_draw_custom_edges() {
    if (current_blueprint.is_null()) return;
    if (edge_meta_cache.empty()) _refresh_edge_cache();

    TypedArray<Dictionary> edges = current_blueprint->get_edges();
    
    // Zoom and pan scalars matching standard Godot GraphEdit behavior
    float zoom = get_zoom();
    Vector2 scroll = get_scroll_offset();

    for (int i = 0; i < edges.size(); ++i) {
        Dictionary e = edges[i];
        StringName from_name = e["from"];
        int from_port = e["from_port"];
        StringName to_name = e["to"];
        int to_port = e["to_port"];

        MemoryGraphNode* from_n = Object::cast_to<MemoryGraphNode>(get_node_or_null(NodePath(from_name)));
        MemoryGraphNode* to_n = Object::cast_to<MemoryGraphNode>(get_node_or_null(NodePath(to_name)));

        if (!from_n || !to_n) continue;

        // Obtain node local positions and translate to canvas space
        Vector2 from_pos = (from_n->get_position_offset() + from_n->get_output_port_position(from_port) - scroll) * zoom;
        Vector2 to_pos = (to_n->get_position_offset() + to_n->get_input_port_position(to_port) - scroll) * zoom;

        // Spline curvature controls
        float distance = Math::abs(to_pos.x - from_pos.x);
        Vector2 cp_offset(Math::max(distance * 0.4f, 50.0f * zoom), 0);
        Vector2 p1 = from_pos + cp_offset;
        Vector2 p2 = to_pos - cp_offset;

        uint64_t h = _hash_edge(from_name, from_port, to_name, to_port);
        EdgeMetadata meta = edge_meta_cache[h];

        // Draw Mutative States over the default GraphEdit connections
        if (meta.is_error) {
            // Error Flag at Midpoint
            Vector2 mid = _evaluate_bezier(from_pos, p1, p2, to_pos, 0.5f);
            Color error_color = get_theme_color("error_indicator_color", "GraphEdit");
            int error_radius = get_theme_constant("error_indicator_radius", "GraphEdit");
            draw_circle(mid, error_radius * zoom, error_color);
            Ref<Font> f = get_theme_font("title_font", "GraphEdit");
            int f_size = get_theme_constant("error_text_size", "GraphEdit");
            Color error_text_color = get_theme_color("error_text_color", "GraphEdit");
            if (f.is_valid()) draw_string(f, mid + Vector2(-4 * zoom, 4 * zoom), "!", HORIZONTAL_ALIGNMENT_CENTER, -1, f_size * zoom, error_text_color);
            
        } else if (meta.access_mode == core::BufferAccessMode::WRITE) {
            // Dashed-Red Line
            const int segments = 20;
            Vector2 last_pt = from_pos;
            for (int s = 1; s <= segments; ++s) {
                float t = static_cast<float>(s) / segments;
                Vector2 next_pt = _evaluate_bezier(from_pos, p1, p2, to_pos, t);
                if (s % 2 != 0) { // Dash skip
                    Color line_color = get_theme_color("write_edge_color", "GraphEdit");
                    int line_width = get_theme_constant("write_edge_thickness", "GraphEdit");
                    draw_line(last_pt, next_pt, line_color, line_width * zoom, true);
                }
                last_pt = next_pt;
            }

        } else if (meta.is_atomic) {
            // Warning-Chevrons (Yellow Dotted)
            const int segments = 15;
            for (int s = 1; s <= segments; ++s) {
                float t = static_cast<float>(s) / segments;
                Vector2 pt = _evaluate_bezier(from_pos, p1, p2, to_pos, t);
                int pt_radius = get_theme_constant("atomic_edge_dot_radius", "GraphEdit");
                Color atomic_color = get_theme_color("atomic_edge_color", "GraphEdit");
                draw_circle(pt, pt_radius * zoom, atomic_color);
            }
        }
    }
}

// ==========================================
// TELEMETRY ROUTING
// ==========================================

void MemoryGraphEdit::set_telemetry_mapping(const Dictionary& p_map) {
    dod_to_ui_map.clear();
    Array keys = p_map.keys();
    
    for (int i = 0; i < keys.size(); ++i) {
        Variant key = keys[i];
        Variant value = p_map[key];
        
        if (key.get_type() == Variant::INT && (value.get_type() == Variant::STRING || value.get_type() == Variant::STRING_NAME)) {
            uint32_t core_id = static_cast<uint32_t>(static_cast<int>(key));
            dod_to_ui_map[core_id] = value;
        }
    }
}

void MemoryGraphEdit::push_telemetry(int p_core_id, const Ref<MemoryGrantInspector>& p_inspector) {
    uint32_t core_id = static_cast<uint32_t>(p_core_id);
    
    auto it = dod_to_ui_map.find(core_id);
    if (it != dod_to_ui_map.end()) {
        StringName ui_node_name = it->second;
        Node* n = get_node_or_null(NodePath(ui_node_name));
        MemoryGraphNode* memory_node = Object::cast_to<MemoryGraphNode>(n);
        
        if (memory_node) {
            memory_node->update_telemetry(p_inspector);
        }
    }
}

void MemoryGraphEdit::_spawn_node_by_type(int p_type_id) {
    if (current_blueprint.is_null()) return;

    // Use strongly-typed resource allocation instead of variants
    Ref<MemoryGraphNodeResource> new_mem_res;
    new_mem_res.instantiate();
    
    Vector2 spawn_offset = (popup_position + get_scroll_offset()) / get_zoom();
    new_mem_res->set_position_offset(spawn_offset);
    new_mem_res->set_type_id(p_type_id);
    
    // In a full implementation, you map p_type_id to the specific buffer/view string name 
    String unique_name = "MemoryNode_" + String::num_int64(UtilityFunctions::randi() % 10000);
    new_mem_res->set_node_name(unique_name);
    

    // --- AUTOMATED PROPAGATION FLOW ---
    // Check if we originated out of a click-drag sequence from a valid existing node
    if (!drag_source_node.is_empty()) {
        Node* parent_node = get_node_or_null(NodePath(drag_source_node));
        MemoryGraphNode* parent_ui = Object::cast_to<MemoryGraphNode>(parent_node);
        
        if (parent_ui) {
            Ref<MemoryGraphNodeResource> parent_res = parent_ui->get_memory_node_resource();
            if (parent_res.is_valid()) {
                
                // Inheriting sequence setup: Flip child mode to FORKED to shadow parent allocation space
                new_mem_res->set_derivation_mode(MemoryGraphNodeResource::MODE_FORKED);
                
                // Propagate the actual asset configuration reference downward
                if (parent_res->get_memory_grant().is_valid()) {
                    new_mem_res->set_memory_grant(parent_res->get_memory_grant());
                }
                
                // Automatically establish the connecting data edge inside the blueprint context
                Dictionary edge;
                edge["from"] = drag_source_node;
                edge["from_port"] = drag_source_port;
                edge["to"] = unique_name;
                edge["to_port"] = 0; // Assume first input port by default on target
                edge["access_mode"] = static_cast<int>(core::BufferAccessMode::READ); // Safe fallback default
                
                current_blueprint->action_add_node(new_mem_res);
                current_blueprint->action_add_edge(edge);
                
                // Reset tracking markers
                drag_source_node = StringName();
                drag_source_port = -1;
                
                _refresh_edge_cache();
                queue_redraw();
                return;
            }
        }
    }

    // Default standalone fallback instantiation pathway
    current_blueprint->action_add_node(new_mem_res);
    
    // Reset tracking context flags safely
    drag_source_node = StringName();
    drag_source_port = -1;
}

void MemoryGraphEdit::_on_node_memory_grant_requested(godot::Object* p_node) {
    MemoryGraphNode* requesting_node = Object::cast_to<MemoryGraphNode>(p_node);
    if (!requesting_node) return;

    Ref<MemoryGraphResource> mem_blueprint = current_blueprint;
    if (mem_blueprint.is_null()) return;

    Ref<MemoryManagerResource> manager = mem_blueprint->get_memory_manager();
    if (manager.is_null()) return;

    // 1. Lazy-initialize the cached window instance if it doesn't exist yet
    if (!grant_request_window) {
        grant_request_window = memnew(GrantRequestWindow);
        add_child(grant_request_window);
        
        // Connect the window's submission back to our orchestrator method
        grant_request_window->connect("grant_payload_submitted", Callable(this, "_on_grant_window_payload_submitted"));
    }

    // 2. Extract authoritative string name lists from our schemas
    TypedArray<StringName> names = manager->get_buffer_names();

    // 3. Clear out old entries and map the window to our current requesting node context
    // We pass the requesting node's unique UI name so we can re-find it upon submission callback.
    grant_request_window->clear_and_popup(requesting_node->get_name(), names);
}

// The target callback that catches the data payload when the user clicks "Request Grant"
void MemoryGraphEdit::_on_grant_window_payload_submitted(const StringName& p_node_name, const PackedInt32Array& p_buffer_ids) {
    godot::UtilityFunctions::print("Received grant request submission for node: ", p_node_name);
    Node* target_node = get_node_or_null(NodePath(p_node_name));
    MemoryGraphNode* memory_node = Object::cast_to<MemoryGraphNode>(target_node);
    if (!memory_node) return;

    Ref<MemoryGraphResource> mem_blueprint = current_blueprint;
    if (mem_blueprint.is_null()) return;

    Ref<MemoryManagerResource> manager = mem_blueprint->get_memory_manager();
    if (manager.is_null()) return;

    // 1. Submit the payload array to our factory method
    Ref<MemoryGrantResource> compiled_grant = manager->request_emulated_grant(p_buffer_ids);

    // 2. Evaluate the result token
    if (compiled_grant.is_valid()) {
        godot::UtilityFunctions::print("Compiled Grant is valid.");
        // Apply the newly authenticated configuration layout directly to the node resource
        memory_node->receive_memory_grant(compiled_grant);
        
        // Trigger a visual refresh of the canvas element
        memory_node->queue_redraw();
        
        UtilityFunctions::print_rich("[color=green]Successfully applied valid MemoryGrantResource to node " + String(p_node_name) + "![/color]");
    } else {
        // Handle validation rejection gracefully in the UI
        memory_node->set_header_state(MemoryGraphNode::HEADER_ERROR);
        memory_node->queue_redraw();
        
        UtilityFunctions::print_rich("[color=red]Failed to assign Grant to node " + String(p_node_name) + ": Validation Constraint Violation.[/color]");

        
    }
}

void MemoryGraphEdit::_on_connection_to_empty(const godot::StringName &p_from_node, int p_from_port, const godot::Vector2 &p_release_position) {
    // Cache the dragging source context immediately before showing the type context popup menu
    drag_source_node = p_from_node;
    drag_source_port = p_from_port;
    
    // Cache screen coordinates for placement calculations
    memory_popup_position = p_release_position; 
    
    _show_popup(p_release_position);
}

} // namespace ideam::godot_ext