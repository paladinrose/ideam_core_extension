#ifndef IDEAM_CORE_BOUNDS_EXTRACTION_LOGIC_H
#define IDEAM_CORE_BOUNDS_EXTRACTION_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <limits>
#include <type_traits>

namespace ideam::core {

template <typename T>
struct BoundsResult {
    T min_bounds;
    T max_bounds;
    T centroid;
    int64_t element_count = 0;
};

/**
 * BoundsExtractionLogic<T>
 * Performs a high-speed read-only reduction to find the AABB and Centroid of a selection.
 * Can write to a single output pointer (Graph Port) or broadcast to a secondary Buffer View.
 */
template <typename T>
struct BoundsExtractionLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // Hint: Read-only for the primary buffer. Write happens to the output_destination.
    static constexpr LogicRequirement requirements = LogicRequirement::READ_ONLY_DATA;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    // --- Configuration ---
    // Option A: Write to a single memory address (e.g., a TaskGraph Constant Port)
    BoundsResult<T>* output_destination = nullptr;

    // Option B: If true, broadcasts the final AABB to a secondary View mapped in Part 1 of the Grant
    bool broadcast_to_elements = false;

    template <typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const {
        // We use execute_sim instead of execute_cull because we aren't modifying the selection mask
        const GrantPartPOD* part = p_context.get_grant_part(p_view.grant_part_index);
        const MemoryBufferSelectionPOD& r_sel = part->selection;

        if (r_sel.element_count == 0) return;

        BoundsResult<T> local_result;
        _init_bounds(local_result.min_bounds, local_result.max_bounds);

        // 1. Core Reduction Loop
        if (r_sel.mode == SelectionMode::DENSE) {
            _reduce_dense(r_sel, p_view, local_result);
        } else if (r_sel.mode == SelectionMode::SPARSE) {
            _reduce_sparse(r_sel, p_view, local_result);
        } else if (r_sel.mode == SelectionMode::RANGE) {
            _reduce_range(r_sel, p_view, local_result);
        }

        // Finalize Centroid
        if (local_result.element_count > 0) {
            local_result.centroid = local_result.centroid / static_cast<float>(local_result.element_count);
        }

        // 2. Output Resolution (Option A)
        if (output_destination) {
            *output_destination = local_result;
        }

        // 3. Broadcast Resolution (Option B)
        if (broadcast_to_elements) {
            T_View output_view = p_context.get_view_for_part<T_View>(1);
            _broadcast(r_sel, output_view, local_result.centroid); // Example: Broadcasting centroid
        }
    }

private:
    // --- Vector Initialization Traits ---
    inline void _init_bounds(T& r_min, T& r_max) const noexcept {
        constexpr float MAX_F = std::numeric_limits<float>::max();
        constexpr float MIN_F = std::numeric_limits<float>::lowest();
        
        if constexpr (requires { r_min.x; r_min.y; r_min.z; r_min.w; }) { // Vector4
            r_min = T(MAX_F, MAX_F, MAX_F, MAX_F); r_max = T(MIN_F, MIN_F, MIN_F, MIN_F);
        } else if constexpr (requires { r_min.x; r_min.y; r_min.z; }) {   // Vector3
            r_min = T(MAX_F, MAX_F, MAX_F); r_max = T(MIN_F, MIN_F, MIN_F);
        } else if constexpr (requires { r_min.x; r_min.y; }) {            // Vector2
            r_min = T(MAX_F, MAX_F); r_max = T(MIN_F, MIN_F);
        }
    }

    inline void _accumulate(BoundsResult<T>& r_res, const T& p_val) const noexcept {
        // Assuming Godot Math Vectors with .min() and .max() methods.
        r_res.min_bounds = r_res.min_bounds.min(p_val);
        r_res.max_bounds = r_res.max_bounds.max(p_val);
        r_res.centroid += p_val;
    }

    // --- High-Speed Reduction Loops ---

    template <typename T_View>
    inline void _reduce_dense(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, BoundsResult<T>& r_res) const {
        const uint64_t* bitset = r_sel.data.bitset;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                _accumulate(r_res, p_view[i]);
                r_res.element_count++;
            }
        }
    }

    template <typename T_View>
    inline void _reduce_sparse(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, BoundsResult<T>& r_res) const {
        const int64_t* indices = r_sel.data.indices;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            _accumulate(r_res, p_view[indices[i]]);
        }
        r_res.element_count = count;
    }

    template <typename T_View>
    inline void _reduce_range(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, BoundsResult<T>& r_res) const {
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            _accumulate(r_res, p_view[i]);
        }
        r_res.element_count = r_sel.element_count;
    }

    // --- Optional Broadcast ---
    template <typename T_View>
    inline void _broadcast(const MemoryBufferSelectionPOD& r_sel, const T_View& p_out_view, const T& p_val) const {
        if (r_sel.mode == SelectionMode::DENSE) {
            for (int64_t i = 0; i < r_sel.capacity; ++i) {
                if (r_sel.data.bitset[i >> 6] & (1ULL << (i & 63))) p_out_view.center_ptr_at(i) = p_val; // Assuming valid mutable assignment
            }
        } else if (r_sel.mode == SelectionMode::SPARSE) {
            for (int64_t i = 0; i < r_sel.element_count; ++i) p_out_view[r_sel.data.indices[i]] = p_val;
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BOUNDS_EXTRACTION_LOGIC_H