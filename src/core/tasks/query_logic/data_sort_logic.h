#ifndef IDEAM_CORE_DATA_SORT_LOGIC_H
#define IDEAM_CORE_DATA_SORT_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <vector>
#include <algorithm>
#include <type_traits>

namespace ideam::core {

enum class SortDirection : uint8_t {
    ASCENDING,
    DESCENDING
};

/**
 * DataSortLogic<T>
 * Generates a sorted index map based on element values or vector magnitudes.
 * Leaves the source selection strictly intact to preserve SoA cache coherency.
 */
template <typename T>
struct DataSortLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // Read-only operation. Does not modify the selection mask or shadow metadata.
    static constexpr LogicRequirement requirements = LogicRequirement::READ_ONLY_DATA;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    // --- Configuration ---
    SortDirection direction = SortDirection::ASCENDING;
    
    // Output Graph Port: Will contain the sorted sequence of buffer indices
    std::vector<int64_t>* output_destination = nullptr;

    template <typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!output_destination) return;

        const GrantPartPOD* part = p_context.get_grant_part(p_view.grant_part_index);
        const MemoryBufferSelectionPOD& r_sel = part->selection;

        if (r_sel.element_count == 0) {
            output_destination->clear();
            return;
        }

        // 1. Gather active indices into the output buffer
        output_destination->clear();
        output_destination->reserve(r_sel.element_count);

        if (r_sel.mode == SelectionMode::DENSE) {
            const uint64_t* bitset = r_sel.data.bitset;
            for (int64_t i = 0; i < r_sel.capacity; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) output_destination->push_back(i);
            }
        } else if (r_sel.mode == SelectionMode::SPARSE) {
            const int64_t* indices = r_sel.data.indices;
            output_destination->assign(indices, indices + r_sel.element_count);
        } else if (r_sel.mode == SelectionMode::RANGE) {
            for (int64_t i = 0; i < r_sel.element_count; ++i) {
                output_destination->push_back(r_sel.start_index + i);
            }
        }

        // 2. Sort the indices by looking up their values in the View
        // We use std::stable_sort to maintain deterministic rendering/processing order for tied values.
        std::stable_sort(output_destination->begin(), output_destination->end(), 
            [this, &p_view](int64_t a, int64_t b) {
                return _compare(p_view[a], p_view[b]);
            }
        );
    }

private:
    [[nodiscard]] inline bool _compare(const T& p_val_a, const T& p_val_b) const noexcept {
        if constexpr (std::is_arithmetic_v<T>) {
            // Scalars: Direct comparison
            return (direction == SortDirection::ASCENDING) ? (p_val_a < p_val_b) : (p_val_a > p_val_b);
        } 
        else if constexpr (requires { p_val_a.length_squared(); }) {
            // Godot Vectors (Vector2, Vector3, etc.): Use magnitude squared to avoid sqrt overhead
            const float mag_a = p_val_a.length_squared();
            const float mag_b = p_val_b.length_squared();
            return (direction == SortDirection::ASCENDING) ? (mag_a < mag_b) : (mag_a > mag_b);
        }
        else {
            return false; // Fallback for unsupported types
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DATA_SORT_LOGIC_H