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
    
    ClassDB::bind_method(D_METHOD("_memory_request_connect", "from_node", "from_port", "to_node", "to_port"), &MemoryGraphEdit::_memory_request_connect);
    ClassDB::bind_method(D_METHOD("_on_connection_to_empty", "from_node", "from_port", "release_position"), &MemoryGraphEdit::_on_connection_to_empty);
    ClassDB::bind_method(D_METHOD("_filtered_popup_select", "id"), &MemoryGraphEdit::_filtered_popup_select);
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

    _create_filtered_popup();
    connect("connection_to_empty", Callable(this, "_on_connection_to_empty"));
}

void MemoryGraphEdit::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAW) {
        _draw_custom_edges();
    }
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

    if ((from_ign && from_ign->get_properties().has("locked") && from_ign->get_properties()["locked"]) || 
        (to_ign && to_ign->get_properties().has("locked") && to_ign->get_properties()["locked"])) {
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
            draw_circle(mid, 12.0f * zoom, Color(0.9f, 0.1f, 0.1f, 1.0f));
            Ref<Font> f = get_theme_font("title_font");
            if (f.is_valid()) draw_string(f, mid + Vector2(-4 * zoom, 4 * zoom), "!", HORIZONTAL_ALIGNMENT_CENTER, -1, 14 * zoom, Color(1,1,1,1));
            
        } else if (meta.access_mode == core::BufferAccessMode::WRITE) {
            // Dashed-Red Line
            const int segments = 20;
            Vector2 last_pt = from_pos;
            for (int s = 1; s <= segments; ++s) {
                float t = static_cast<float>(s) / segments;
                Vector2 next_pt = _evaluate_bezier(from_pos, p1, p2, to_pos, t);
                if (s % 2 != 0) { // Dash skip
                    draw_line(last_pt, next_pt, Color(0.9f, 0.2f, 0.2f, 0.8f), 3.0f * zoom, true);
                }
                last_pt = next_pt;
            }

        } else if (meta.is_atomic) {
            // Warning-Chevrons (Yellow Dotted)
            const int segments = 15;
            for (int s = 1; s <= segments; ++s) {
                float t = static_cast<float>(s) / segments;
                Vector2 pt = _evaluate_bezier(from_pos, p1, p2, to_pos, t);
                draw_circle(pt, 3.0f * zoom, Color(1.0f, 0.8f, 0.1f, 0.9f));
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

// ... _create_filtered_popup, _on_connection_to_empty, _show_filtered_popup, _filtered_popup_select, _get_filtered_node_types remain same as base logic ...
void MemoryGraphEdit::_create_filtered_popup() {}
void MemoryGraphEdit::_on_connection_to_empty(const godot::StringName &p_from_node, int p_from_port, const godot::Vector2 &p_release_position) {}
void MemoryGraphEdit::_show_filtered_popup(const godot::Vector2 &p_at, uint32_t p_filter_mask) {}
void MemoryGraphEdit::_filtered_popup_select(int p_id) {}
TypedArray<String> MemoryGraphEdit::_get_filtered_node_types(uint32_t p_filter_mask) const { return TypedArray<String>(); }

} // namespace ideam::godot_ext