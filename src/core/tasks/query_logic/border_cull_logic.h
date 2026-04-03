#ifndef IDEAM_CORE_BORDER_CULL_LOGIC_H
#define IDEAM_CORE_BORDER_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <algorithm>
#include <vector>

namespace ideam::core {

/**
 * BorderCullLogic
 * Identifies "border" elements: those that are either physically on the edge 
 * of the grid or have at least one neighbor missing from the current selection.
 * CULL: Strips the halo (keeps strictly interior elements).
 * ADD: Spatially dilates the selection by queueing unselected neighbors.
 */
template <typename T, typename T_Strategy, uint32_t DimCount>
struct BorderCullLogic {
    using ValueType       = T; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StencilView<T, T_Strategy, DimCount>;

    static constexpr LogicRequirement requirements = LogicRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) const {
        
        if (r_selection.mode != SelectionMode::SPARSE) return;

        if constexpr (Op == QueryOp::CULL) {
            _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_dilation(r_selection, p_view, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }

private:
    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
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

            // Halo Stripping: We only retain elements that are NOT borders.
            if (!is_border) {
                r_selection.data.indices[write_ptr++] = current_idx;
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_dilation(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        std::vector<int64_t> sorted_indices(r_selection.data.indices, r_selection.data.indices + r_selection.element_count);
        std::sort(sorted_indices.begin(), sorted_indices.end());

        const int64_t original_count = r_selection.element_count;
        const T_Strategy& strategy = p_view.get_strategy();
        const uint64_t* unclaimed = r_selection.unclaimed_mask;

        for (int64_t i = 0; i < original_count; ++i) {
            const int64_t current_idx = r_selection.data.indices[i];

            for (uint32_t d = 0; d < DimCount; ++d) {
                for (int32_t step : {-1, 1}) {
                    int64_t neighbor_idx = strategy.get_neighbor_index(current_idx, d, step);

                    // If neighbor index exists but isn't part of our selection, it's a dilation candidate
                    if (neighbor_idx != -1 && !std::binary_search(sorted_indices.begin(), sorted_indices.end(), neighbor_idx)) {
                        
                        // Check if the neighbor is globally unclaimed/available before queuing
                        if (unclaimed && (unclaimed[neighbor_idx >> 6] & (1ULL << (neighbor_idx & 63)))) {
                            p_ctx.queue_selection_command(target_buffer_id, neighbor_idx);
                        }
                    }
                }
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BORDER_CULL_LOGIC_H