#ifndef IDEAM_CORE_EROSION_CULL_LOGIC_H
#define IDEAM_CORE_EROSION_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <vector>

namespace ideam::core {

/**
 * ErosionCullLogic
 * A morphological pruning logic. An element is removed (eroded) if any of 
 * its neighbors are missing from the current selection.
 * * MAGIC: Converts the legacy search-heavy erosion into a snapshot-based 
 * spatial prune using StencilView strides.
 */
template <typename T, typename T_Strategy, uint32_t DimCount>
struct ErosionCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StencilView<T, T_Strategy, DimCount>;

    // Erosion is strictly spatial and requires neighborhood awareness.
    static constexpr LogicRequirement requirements = LogicRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;

    // --- Configuration Data ---
    // Number of times to apply the erosion pass in a single task execution.
    int32_t iterations = 1; 

    /**
     * execute_cull
     * The DOD entry point. Performs morphological erosion on the selection set.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (iterations <= 0) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            // For morphological operations, sparse sets are converted to dense 
            // bitsets to prevent O(N log N) neighbor lookups.
            SelectionUtils::convert_to_dense(r_selection);
            _cull_dense(r_selection, p_view);
        }
    }

private:
    /**
     * _evaluate_neighbor
     * Checks if a specific neighbor is currently "on" in the selection snapshot.
     */
    inline bool _evaluate_neighbor(int64_t p_neighbor_idx, const uint64_t* p_snapshot, int64_t p_capacity) const {
        // Physical Border Check: If neighbor is out of bounds, the center erodes
        if (p_neighbor_idx == -1) return false;

        // Selection Check: Is the neighbor bit active in the snapshot?
        return (p_snapshot[p_neighbor_idx >> 6] & (1ULL << (p_neighbor_idx & 63)));
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t capacity = r_selection.capacity;
        const T_Strategy& strategy = p_view.get_strategy();

        // Erosion pass loop
        for (int32_t iter = 0; iter < iterations; ++iter) {
            // Snapshotting: We must read from the previous state while writing to the new one
            // to prevent erosion from cascading incorrectly across the grid in one pass.
            std::vector<uint64_t> snapshot(bitset, bitset + ((capacity + 63) / 64));
            const uint64_t* snap_ptr = snapshot.data();

            for (int64_t i = 0; i < capacity; ++i) {
                // If the cell is already off, skip.
                if (!(snap_ptr[i >> 6] & (1ULL << (i & 63)))) continue;

                bool should_erode = false;

                // Check all adjacent neighbors (Von Neumann neighborhood)
                for (uint32_t d = 0; d < DimCount; ++d) {
                    // Check positive neighbor (+1)
                    int64_t next_idx = strategy.get_neighbor_index(i, d, 1);
                    if (!_evaluate_neighbor(next_idx, snap_ptr, capacity)) {
                        should_erode = true;
                        break;
                    }

                    // Check negative neighbor (-1)
                    int64_t prev_idx = strategy.get_neighbor_index(i, d, -1);
                    if (!_evaluate_neighbor(prev_idx, snap_ptr, capacity)) {
                        should_erode = true;
                        break;
                    }
                }

                if (should_erode) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }

            // Early out if we've eroded the entire selection
            if (r_selection.element_count <= 0) break;
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_EROSION_CULL_LOGIC_H