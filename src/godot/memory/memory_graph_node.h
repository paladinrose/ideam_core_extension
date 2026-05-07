#pragma once

#include "../graphs/ideam_graph_node.h"
#include "memory_inspectors.h"
#include "../../core/memory/memory_buffer_pod.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/texture2d.hpp>
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
    godot::Button* inspect_memory_btn = nullptr;

    // Active visual states
    LayoutHeaderState header_state = HEADER_VALID;
    TelemetryBadgeState telemetry_state = TELEMETRY_INACTIVE;

    // Internal Helpers for Theme mapping
    godot::Ref<godot::Texture2D> _get_icon_for_layout(core::BufferLayoutType p_layout) const;
    godot::Ref<godot::Texture2D> _get_badge_icon_for_telemetry(TelemetryBadgeState p_state) const;

protected:
    // Maps port_index -> Trait Bitmask
    std::unordered_map<int, uint32_t> input_port_signatures;
    std::unordered_map<int, uint32_t> output_port_signatures;

    static void _bind_methods();
    virtual void _build_ui() override;
    
    // Intercept draw to render headers and telemetry directly over the node
    void _notification(int p_what);

    void _on_inspect_memory_pressed();

public:
    MemoryGraphNode();
    virtual ~MemoryGraphNode() override = default;

    /**
     * @brief Helper for derived classes to declare their compile-time traits to the UI.
     * Should be called inside overridden _build_ui().
     */
    void register_port_signature(int p_port_idx, bool p_is_output, uint32_t p_trait_mask);
    uint32_t get_port_signature(int p_port_idx, bool p_is_output) const;

    // --- Tier 2: State Mutations ---
    void update_telemetry(const godot::Ref<MemoryGrantInspector>& p_inspector);
    void set_header_state(LayoutHeaderState p_state);
    
    /**
     * @brief Binds a BufferLayoutType to a specific visual port slot.
     */
    void update_memory_port(int p_slot_index, bool p_is_left, core::BufferLayoutType p_layout);

    void receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names);
};

} // namespace ideam::godot_ext