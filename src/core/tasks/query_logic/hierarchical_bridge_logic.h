#ifndef IDEAM_CORE_HIERARCHICAL_BRIDGE_LOGIC_H
#define IDEAM_CORE_HIERARCHICAL_BRIDGE_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/bridge_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

/**
 * HierarchicalBridgeLogic
 * Target: Micro-cells (FLAT). Source: Macro-cells (TILED_SOA).
 * Expands a single parent selection into a block of active children.
 */
struct HierarchicalBridgeLogic {
    using ValueType       = uint8_t; 
    using DefaultStrategy = FlatStrategy;
    // Assuming BridgeView binds a Parent type to a Child type
    using DefaultView     = BridgeView<uint8_t, uint8_t, 1, DefaultStrategy>; 

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::FLAT;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* parent_selection = nullptr;
    uint32_t target_buffer_id = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) const {
        
        if (!parent_selection || parent_selection->mode != SelectionMode::DENSE) return;

        if constexpr (Op == QueryOp::CULL) {
            _cull_dense(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _has_active_parent(int64_t child_idx, const T_View& p_view) const {
        int64_t parent_idx = p_view.get_parent_index(child_idx);
        if (parent_idx < 0 || parent_idx >= parent_selection->capacity) return false;
        return (parent_selection->data.bitset[parent_idx >> 6] & (1ULL << (parent_idx & 63))) != 0;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_has_active_parent(i, p_view)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t child_idx = (w << 6) + bit_index;
                
                if (child_idx >= r_selection.capacity) break;

                if (_has_active_parent(child_idx, p_view)) {
                    p_ctx.queue_selection_command(target_buffer_id, child_idx);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_HIERARCHICAL_BRIDGE_LOGIC_H