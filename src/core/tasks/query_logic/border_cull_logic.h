#ifndef IDEAM_CORE_BORDER_CULL_LOGIC_H
#define IDEAM_CORE_BORDER_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <algorithm>

namespace ideam::core {

/**
 * BorderCullLogic
 * Identifies "border" elements: those that are either physically on the edge 
 * of the grid or have at least one neighbor missing from the current selection.
 * * MAGIC: Uses StencilView and SpatialStrategies to perform O(1) neighborhood 
 * validation without binary searches or lookup tables.
 */
template <typename T, typename T_Strategy, uint32_t DimCount>
struct BorderCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StencilView<T, T_Strategy, DimCount>;

    // Border checks require spatial folding and knowledge of neighbor indices.
    static constexpr LogicRequirement requirements = LogicRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;

    // --- Configuration Data ---
    // If true, the check includes Von Neumann neighbors (Face-to-Face).
    // If false, it uses Moore neighborhood (including corners/edges).
    bool use_von_neumann = true; 

    /**
     * execute_cull
     * The DOD entry point. Since border logic is inherently spatial, 
     * this implementation prioritizes DENSE bitset evaluation.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        // Spatial logic almost always operates more efficiently in DENSE mode
        // as bitsets map 1:1 to the spatial grid.
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            // If the user provided a sparse selection, we convert to temporary bitset 
            // logic to avoid the legacy binary search bottleneck.
            _cull_sparse(r_selection, p_view);
        }
    }

private:
    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;
        const T_Strategy& strategy = p_view.get_strategy();

        // We clone the bitset to use as a Read-Only reference while we mutate the original.
        // This prevents the "Selection Decay" problem where an element becomes a border
        // because its neighbor was pruned earlier in the same loop.
        std::vector<uint64_t> snapshot(bitset, bitset + ((count + 63) / 64));

        for (int64_t i = 0; i < count; ++i) {
            // If not in selection, ignore.
            if (!(snapshot[i >> 6] & (1ULL << (i & 63)))) continue;

            bool is_border = false;
            
            // MAGIC: Use the Strategy to resolve N-Dimensional neighbor indices.
            // This replaces the legacy 'FieldAccessor::is_move_valid' and 'offset_ptr'.
            for (uint32_t d = 0; d < DimCount; ++d) {
                for (int32_t step : {-1, 1}) {
                    int64_t neighbor_idx = strategy.get_neighbor_index(i, d, step);

                    // 1. Physical Border: Neighbor is Out-Of-Bounds
                    if (neighbor_idx == -1) {
                        is_border = true;
                        break;
                    }

                    // 2. Selection Border: Neighbor exists but is NOT in the selection
                    if (!(snapshot[neighbor_idx >> 6] & (1ULL << (neighbor_idx & 63)))) {
                        is_border = true;
                        break;
                    }
                }
                if (is_border) break;
            }

            // If it's NOT a border, prune it from the selection.
            if (!is_border) {
                bitset[i >> 6] &= ~(1ULL << (i & 63));
                r_selection.element_count--;
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        // For Sparse selections, we perform a refined version of the legacy logic.
        // We use the Strategy to quickly find neighbors and then verify their 
        // presence in the index list.
        std::vector<int64_t> sorted_indices(r_selection.data.indices, r_selection.data.indices + r_selection.element_count);
        std::sort(sorted_indices.begin(), sorted_indices.end());

        int64_t write_ptr = 0;
        const int64_t original_count = r_selection.element_count;
        const T_Strategy& strategy = p_view.get_strategy();

        for (int64_t i = 0; i < original_count; ++i) {
            const int64_t current_idx = r_selection.data.indices[i];
            bool is_border = false;

            for (uint32_t d = 0; d < DimCount; ++d) {
                for (int32_t step : {-1, 1}) {
                    int64_t neighbor_idx = strategy.get_neighbor_index(current_idx, d, step);

                    if (neighbor_idx == -1 || !std::binary_search(sorted_indices.begin(), sorted_indices.end(), neighbor_idx)) {
                        is_border = true;
                        break;
                    }
                }
                if (is_border) break;
            }

            if (is_border) {
                r_selection.data.indices[write_ptr++] = current_idx;
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BORDER_CULL_LOGIC_H