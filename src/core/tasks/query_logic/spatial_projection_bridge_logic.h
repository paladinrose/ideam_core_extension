#ifndef IDEAM_CORE_SPATIAL_PROJECTION_BRIDGE_LOGIC_H
#define IDEAM_CORE_SPATIAL_PROJECTION_BRIDGE_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <vector>

namespace ideam::core {

/**
 * SpatialProjectionBridgeLogic<T_Coord, T_Strategy>
 * Target: Field/Grid. Source: Physical Objects (Entities, Mass).
 * Projects Source coordinates into a grid bitmask, then filters the Target Grid.
 * REQUIREMENT: T_View MUST be bound to the Source Buffer.
 */
template <typename T_Coord, typename T_Strategy>
struct SpatialProjectionBridgeLogic {
    using ValueType       = T_Coord; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T_Coord, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* source_selection = nullptr; // Physical objects
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_View_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_source_view) const {
        
        if (!source_selection) return;

        // 1. Build the O(1) projection bitmask (Zero-branching footprint generation)
        std::vector<uint64_t> projection_mask((r_selection.capacity + 63) >> 6, 0ULL);
        
        if (source_selection->mode == SelectionMode::DENSE) {
            const uint64_t* src_bitset = source_selection->data.bitset;
            for (int64_t i = 0; i < source_selection->capacity; ++i) {
                if (src_bitset[i >> 6] & (1ULL << (i & 63))) {
                    int64_t cell_id = p_source_view.get_strategy().get_cell_index(p_source_view[i]);
                    if (cell_id >= 0 && cell_id < r_selection.capacity) {
                        projection_mask[cell_id >> 6] |= (1ULL << (cell_id & 63));
                    }
                }
            }
        } else if (source_selection->mode == SelectionMode::SPARSE) {
            for (int64_t i = 0; i < source_selection->element_count; ++i) {
                int64_t src_idx = source_selection->data.indices[i];
                int64_t cell_id = p_source_view.get_strategy().get_cell_index(p_source_view[src_idx]);
                if (cell_id >= 0 && cell_id < r_selection.capacity) {
                    projection_mask[cell_id >> 6] |= (1ULL << (cell_id & 63));
                }
            }
        }

        // 2. Apply projection to Target Selection
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) {
                uint64_t* target_bits = r_selection.data.bitset;
                const int64_t words = (r_selection.capacity + 63) >> 6;
                for (int64_t w = 0; w < words; ++w) {
                    uint64_t old_word = target_bits[w];
                    target_bits[w] &= projection_mask[w];
                    r_selection.element_count -= std::popcount(old_word ^ target_bits[w]);
                }
            }
        } else if constexpr (Op == QueryOp::ADD) {
            // Addition: Queue target cells that are projected AND globally unclaimed
            const uint64_t* unclaimed = r_selection.unclaimed_mask;
            const int64_t words = (r_selection.capacity + 63) >> 6;
            for (int64_t w = 0; w < words; ++w) {
                uint64_t mask = projection_mask[w];
                if (unclaimed) mask &= unclaimed[w];
                
                while (mask != 0) {
                    int bit_index = std::countr_zero(mask);
                    int64_t global_index = (w << 6) + bit_index;
                    p_context.queue_selection_command(target_buffer_id, global_index);
                    mask &= (mask - 1); 
                }
            }
        }
    }

    template<typename T_View, typename T_View_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SPATIAL_PROJECTION_BRIDGE_LOGIC_H