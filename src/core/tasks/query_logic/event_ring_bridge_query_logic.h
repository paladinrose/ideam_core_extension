#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/ring_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

namespace ideam::core {

/**
 * EventRingBridgeQueryLogic<T_Event>
 * Target: ECS Entities (Sparse). Source: Event Queue (Ring).
 * Drains a Ring buffer and translates the event payloads directly into Entity selections.
 */
template <typename T_Event>
struct EventRingBridgeQueryLogic {
    using ValueType       = T_Event; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = RingView<T_Event, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::QUEUE_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::RING;
    static constexpr DataType required_types              = DataType::CUSTOM;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;
    
    static constexpr bool supports_cull = false;
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0; // The Sparse Set Entity Buffer

    static godot::Array get_ui_properties() {
        return godot::Array();
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        // No properties for this logic, but method must be defined to satisfy the interface.
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        // Ring bridges are strictly additive. You "pull" events to wake entities up.
        if constexpr (Op == QueryOp::ADD) {
            T_Event evt;
            // No SFINAE required. The Registry guarantees p_view is a RingView (or similar queue)
            while (p_view.pop(evt)) {
                if constexpr (requires { evt.entity_id; }) { // Keep the payload trait check
                    p_context.queue_selection_command(target_buffer_id, evt.entity_id);
                }
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_EVENT_RING_BRIDGE_QUERY_LOGIC_H