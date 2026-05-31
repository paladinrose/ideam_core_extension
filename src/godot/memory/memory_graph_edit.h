#pragma once

#include "../graphs/ideam_graph_edit.h"
#include "memory_graph_resource.h"
#include "memory_inspectors.h"
#include "memory_graph_node.h"
#include "memory_graph_node_resource.h"
#include "grant_request_window.h"
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <unordered_map>

namespace ideam::godot_ext {

/**
 * @class MemoryGraphEdit
 * @brief Specialized GraphEdit for Memory topologies.
 * Enforces strict DOD access constraints (Exclusive Writes, Atomic Locks)
 * and physically visualizes memory telemetry across execution edges.
 */
class MemoryGraphEdit : public IdeamGraphEdit {
    GDCLASS(MemoryGraphEdit, IdeamGraphEdit)

private:
    // Reverse lookup map: Core NodeID -> UI StringName
    std::unordered_map<uint32_t, godot::StringName> dod_to_ui_map;

    // Specialized popup for filtered context menus
    godot::PopupMenu* filtered_popup = nullptr;
    uint32_t current_filter_mask = 0;
    godot::Vector2 memory_popup_position;

    GrantRequestWindow* grant_request_window = nullptr;

    // --- Edge Topography Tracking ---
    struct EdgeMetadata {
        core::BufferAccessMode access_mode = core::BufferAccessMode::READ;
        bool is_error = false;
        bool is_atomic = false;
    };
    
    // Hash map for O(1) edge metadata lookup during the hot draw loop
    std::unordered_map<uint64_t, EdgeMetadata> edge_meta_cache;
    uint64_t _hash_edge(const godot::StringName& p_from, int p_from_port, const godot::StringName& p_to, int p_to_port) const;

    // Math helper for drawing custom splines
    godot::Vector2 _evaluate_bezier(const godot::Vector2& p0, const godot::Vector2& p1, const godot::Vector2& p2, const godot::Vector2& p3, float t) const;

protected:
    static void _bind_methods();
    virtual void _notification(int p_what) override;
    virtual void _update_theme_properties() override;

    void _on_connection_to_empty(const godot::StringName &p_from_node, int p_from_port, const godot::Vector2 &p_release_position);
    
    
    
    // Tracking context variables for runtime node spawning inheritances
    godot::StringName drag_source_node;
    int drag_source_port = -1;

    // Incorporate strongly-typed node spawning locally
    void _spawn_node_by_type(int p_type_id) override;

    // ---  Access Routers ---
   
    void _memory_request_connect(const godot::StringName &p_from_node, int p_from_port, const godot::StringName &p_to_node, int p_to_port);
    void _on_grant_window_payload_submitted(const godot::StringName& p_node_name, const godot::PackedInt32Array& p_buffer_ids);
    void _on_node_memory_grant_requested(godot::Object* p_node);
    void _on_buffer_names_requested(godot::Object* p_node, const godot::Array& p_buffer_ids);
    
    // Validation Rules
    bool _validate_write_collision(const godot::StringName &p_to_node, int p_to_port, core::BufferAccessMode p_incoming_mode);
    bool _validate_fork_grant(const godot::StringName &p_from_node, int p_from_port, const godot::StringName &p_to_node, int p_to_port);

    // Visuals
    void _draw_custom_edges();
    void _refresh_edge_cache();

public:
    MemoryGraphEdit();
    virtual ~MemoryGraphEdit() override;

    void _ready() override;

    // --- Telemetry Routing ---
    void set_telemetry_mapping(const godot::Dictionary& p_map);
    void push_telemetry(int p_core_id, const godot::Ref<MemoryGrantInspector>& p_inspector);
};

} // namespace ideam::godot_ext