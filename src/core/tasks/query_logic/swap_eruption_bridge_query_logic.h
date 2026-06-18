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

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::SWAP_ACCESS | ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_NUMERIC | DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::ANY_VECTOR4;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = false; // Eruptions are strictly additive
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;
    uint32_t source_column_id = 0;
    T eruption_threshold;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary col_prop;
        col_prop["name"] = "source_column_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);

        godot::Dictionary thresh_prop;
        thresh_prop["name"] = "eruption_threshold";
        thresh_prop["type"] = "T"; 
        thresh_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(thresh_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_column_id")) {
            source_column_id = static_cast<uint32_t>(p_props["source_column_id"]);
        }
        if (p_props.has("eruption_threshold")) {
            eruption_threshold = static_cast<T>(p_props["eruption_threshold"]);
        }
    }
    
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
// --- The DOD Value Adapter ---
    template <typename T_Val>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _safe_cast(const T_Val& p_val) const {
        if constexpr (std::is_pointer_v<T_Val>) {
            return *reinterpret_cast<const T*>(p_val);
        } else if constexpr (requires { static_cast<T>(p_val); }) {
            return static_cast<T>(p_val);
        } else {
            return T{}; 
        }
    }
    
    template <typename T_View>
    void _add_eruptions(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                // 1. Leverage the zero-overhead SwapElementProxy returned by operator[]
                auto proxy = p_view[global_index];
                
                // 2. In double-buffering delta evaluation:
                // read() -> State(T-1) [The baseline]
                // write() -> State(T) [The freshly updated value]
                T previous_val = _safe_cast(proxy.read());
                T current_val  = _safe_cast(proxy.write());
                
                // 3. Concept-Driven Guard: Only compile if T supports subtraction
                // AND the >= operator evaluates to a standard boolean.
                if constexpr (requires(T a, T b) {
                    { a - b };
                    { a >= b } -> std::convertible_to<bool>;
                }) {
                    T delta = current_val - previous_val;
                    if (delta >= eruption_threshold) {
                        p_ctx.queue_selection_command(target_buffer_id, global_index);
                    }
                }
                
                mask &= (mask - 1);
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_SWAP_ERUPTION_BRIDGE_QUERY_LOGIC_H