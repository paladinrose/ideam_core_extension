#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

/**
 * SwapEruptionBridgeQueryLogic<T>
 * Target: Particle/Mass Buffer (ANY_LINEAR). Source: Simulation (SWAP_VIEW).
 * Evaluates the delta between State(T) and State(T-1). Triggers ADD if delta exceeds a threshold.
 */
template <typename T>
struct SwapEruptionBridgeQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY_NUMERIC | DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::ANY_VECTOR4;

    static constexpr bool supports_cull = false; // Eruptions are strictly additive
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;
    uint32_t source_column_id = 0;
    T eruption_threshold;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    /**
     * configure_view
     * Binds the secondary (Write/Previous) state into the SwapView.
     * Assumes ping-pong buffers are packed sequentially in the same Memory Grant.
     */
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { view.bind_secondary(p_part); }) {
            // In a Swap configuration, Part[0] is typically Read, Part[1] is Write.
            // We fetch the adjacent memory block in the array.
            const GrantPartPOD* secondary_part = p_part + 1; 
            view.bind_secondary(secondary_part);
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::ADD) {
            _add_eruptions(r_selection, p_view, p_context);
        }
    }

private:
    template <typename T_View>
    void _add_eruptions(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        // Note: The SwapView naturally exposes .get_current(i) and .get_previous(i)
        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                // Evaluate the simulation delta
                T delta = p_view.get_current(global_index) - p_view.get_previous(global_index);
                if (delta >= eruption_threshold) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_SWAP_ERUPTION_BRIDGE_QUERY_LOGIC_H