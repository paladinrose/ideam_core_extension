#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/bridge_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

struct HierarchicalBridgeQueryLogic {
    using ValueType       = uint8_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<uint8_t, uint8_t, 1, DefaultStrategy>; 

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::ANY;
    
    static constexpr size_t dimensions = 0; 
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    static constexpr size_t transient_workspace_bytes = 0;
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    // --- State Payload ---
    const MemoryBufferSelectionPOD* parent_selection = nullptr;
    uint32_t target_buffer_id = 0;
    uint32_t child_cells_per_parent = 1; // <--- NEW: Decoupled Topological Data

    static godot::Array get_ui_properties() { return godot::Array(); }
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }
    void apply_properties(const godot::Dictionary& p_props) noexcept {}

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { view.bind_secondary(p_part); }) {
            const GrantPartPOD* child_part = p_context.get_grant_part(target_buffer_id);
            view.bind_secondary(child_part);
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!parent_selection) return;

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection); // Removed p_view
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_context); // Removed p_view
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    // Removed p_view parameter completely.
    inline bool _has_active_parent(int64_t child_idx) const {
        // --- DOD PURE MATH ---
        // Translates flat child space directly to flat parent space.
        int64_t parent_idx = child_idx / child_cells_per_parent; 
        
        if (parent_idx < 0 || parent_idx >= parent_selection->capacity) return false;

        // Fetch direct bitmask state (L1 cache friendly sequential read)
        return (parent_selection->data.bitset[parent_idx >> 6] & (1ULL << (parent_idx & 63))) != 0;
    }

    void _cull_dense(MemoryBufferSelectionPOD& r_selection) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_has_active_parent(i)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    void _add_available(const MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t child_idx = (w << 6) + bit_index;
                
                if (child_idx >= r_selection.capacity) break;

                if (_has_active_parent(child_idx)) {
                    p_ctx.queue_selection_command(target_buffer_id, child_idx);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core