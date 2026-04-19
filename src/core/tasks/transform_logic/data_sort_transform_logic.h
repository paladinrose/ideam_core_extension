#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <vector>
#include <algorithm>
#include <type_traits>

namespace ideam::core {

enum class SortDirection : uint8_t {
    ASCENDING,
    DESCENDING
};

/**
 * DataSortTransformLogic<T>
 * Generates a sorted index map based on element values or vector magnitudes.
 * Leaves the source selection strictly intact to preserve SoA cache coherency.
 */
template <typename T>
struct DataSortTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr TransformRequirement requirements = TransformRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY_NUMERIC | DataType::GODOT_VECTOR_TYPES;
    static constexpr size_t transient_workspace_bytes = 0; // Output vector is externally provided via Graph Port

    // --- Configuration ---
    SortDirection direction = SortDirection::ASCENDING;
    uint32_t primary_buffer_id = INVALID_ID;
    
    // Output Graph Port: Will contain the sorted sequence of buffer indices
    std::vector<int64_t>* output_destination = nullptr;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const {
        return primary_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD& context, T_View& p_view) const {
        if (!output_destination) return;

        const MemoryBufferSelectionPOD* sel = context.get_selection(primary_buffer_id);
        if (!sel || !sel->is_valid()) return;

        // 1. Unpack active indices from the selection mask
        output_destination->clear();
        output_destination->reserve(sel->element_count);

        if (sel->mode == SelectionMode::DENSE) {
            const uint64_t* bitset = sel->data.bitset;
            const int64_t cap = sel->capacity;
            for (int64_t i = 0; i < cap; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    output_destination->push_back(i);
                }
            }
        } else if (sel->mode == SelectionMode::SPARSE) {
            const int64_t* indices = sel->data.indices;
            const int64_t count = sel->element_count;
            for (int64_t i = 0; i < count; ++i) {
                output_destination->push_back(indices[i]);
            }
        } else if (sel->mode == SelectionMode::RANGE) {
            for (int64_t i = 0; i < sel->element_count; ++i) {
                output_destination->push_back(sel->start_index + i);
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
        return false;
    }
};

} // namespace ideam::core

 // IDEAM_CORE_DATA_SORT_TRANSFORM_LOGIC_H