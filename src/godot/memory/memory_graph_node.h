#pragma once

#include "../graphs/ideam_graph_node.h"
#include "memory_graph_node_resource.h"
#include "memory_inspectors.h"
#include "../../core/memory/memory_buffer_pod.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <unordered_map>

namespace ideam::godot_ext {

/**
 * MemoryGraphPortTraits
 * Bitmask defining the capability/strategy required or provided by a specific memory graph port.
 * Evaluated at runtime by the Editor UI to prevent invalid topological connections.
 */
enum MemoryGraphPortTraits : uint32_t {
    TRAIT_NONE           = 0,
    TRAIT_LINEAR_ACCESS  = 1 << 0, 
    TRAIT_SPATIAL_ACCESS = 1 << 1, 
    TRAIT_SIMD_ACCESS    = 1 << 2, 
    TRAIT_RANDOM_ACCESS  = 1 << 3, 
    TRAIT_VIRTUAL_MEMORY = 1 << 4,
    TRAIT_IS_SPATIAL     = 1 << 5, 
    TRAIT_IS_PAGED       = 1 << 6  
};

class MemoryGraphNode : public IdeamGraphNode {
    GDCLASS(MemoryGraphNode, IdeamGraphNode)

public:
    enum LayoutHeaderState {
        HEADER_VALID,
        HEADER_ERROR
    };

    enum TelemetryBadgeState {
        TELEMETRY_INACTIVE,
        TELEMETRY_ACTIVE,
        TELEMETRY_DIRTY,
        TELEMETRY_ERROR
    };

private:
    // Telemetry Snapshot (Thread-safe read-only View)
    godot::Ref<MemoryGrantInspector> latest_grant_snapshot;

    // UI Elements
    godot::HBoxContainer* memory_controls_hb = nullptr; 
    godot::Button* inspect_memory_btn = nullptr;
    godot::Button* request_grant_btn = nullptr;
    godot::TextureRect* telemetry_badge = nullptr;

    godot::OptionButton* grant_selector_btn = nullptr;
    godot::TypedArray<MemoryGrantResource> cached_dropdown_grants;

    // Active visual states
    LayoutHeaderState header_state = HEADER_VALID;
    TelemetryBadgeState telemetry_state = TELEMETRY_INACTIVE;

    // Internal Helpers for Theme mapping
    godot::Ref<godot::Texture2D> _get_icon_for_layout(core::BufferLayoutType p_layout) const;
    godot::Ref<godot::Texture2D> _get_telemetry_badge_icon(TelemetryBadgeState p_state) const;

protected:
    // Maps port_index -> Trait Bitmask
    std::unordered_map<int, uint32_t> input_port_signatures;
    std::unordered_map<int, uint32_t> output_port_signatures;

    std::unordered_map<int, core::BufferLayoutType> input_port_layouts;
    std::unordered_map<int, core::BufferLayoutType> output_port_layouts;

    static void _bind_methods();
    virtual void _build_ui() override;
    
    // Intercept draw to render headers and telemetry directly over the node
    void _notification(int p_what);

    virtual void _update_theme_properties() override;
    void _on_inspect_memory_pressed();
    void _on_request_grant_pressed();
    
    void _on_grant_dropdown_about_to_popup();
    void _on_grant_selected(int p_index);

public:
    MemoryGraphNode();
    virtual ~MemoryGraphNode() override = default;

    /**
     * @brief O(1) typed getter for the underlying Memory Resource.
     * Replaces any dynamic Variant lookups for memory configurations.
     */
    godot::Ref<MemoryGraphNodeResource> get_memory_node_resource() const;

    /**
     * @brief Helper for derived classes to declare their compile-time traits to the UI.
     * Should be called inside overridden _build_ui().
     */
    void register_port_signature(int p_port_idx, bool p_is_output, uint32_t p_trait_mask);
    uint32_t get_port_signature(int p_port_idx, bool p_is_output) const;

    virtual void update_from_resource(const godot::Ref<IdeamGraphNodeResource>& p_node_res) override;
    
    // --- Tier 2: State Mutations ---
    void update_telemetry(const godot::Ref<MemoryGrantInspector>& p_inspector);
    void set_header_state(LayoutHeaderState p_state);
    
    /**
     * @brief Binds a BufferLayoutType to a specific visual port slot.
     */
    // [Memz] Wrap the core enum in godot::BitField to satisfy the Variant int64_t cast
    void update_memory_port(int p_slot_index, bool p_is_left, godot::BitField<core::BufferLayoutType> p_layout);

    virtual void receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names);
    virtual void receive_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant);

    void populate_grant_dropdown(const godot::TypedArray<MemoryGrantResource>& p_grants);
    
    virtual void receive_connection_info(const godot::Dictionary& p_info) override;
};

} // namespace ideam::godot_ext

VARIANT_BITFIELD_CAST(ideam::core::BufferLayoutType);
VARIANT_ENUM_CAST(ideam::godot_ext::MemoryGraphNode::LayoutHeaderState);
VARIANT_ENUM_CAST(ideam::godot_ext::MemoryGraphNode::TelemetryBadgeState);