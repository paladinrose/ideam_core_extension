#ifndef IDEAM_CORE_DISTANCE_CULL_LOGIC_H
#define IDEAM_CORE_DISTANCE_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cmath>
#include <type_traits>

namespace ideam::core {

/**
 * DistanceCullLogic<T>
 * Filters elements based on their distance from a center point.
 * T: The data type (float, Vector2, Vector3, Vector4, Color).
 * MAGIC: Uses squared distance comparisons to avoid expensive sqrt() calls.
 */
template <typename T>
struct DistanceCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    enum class Comparison : uint8_t {
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL
    };

    // --- Configuration Data ---
    uint32_t column_id   = 0;
    T target_center      = T{};     // Directly typed center point
    float threshold      = 1.0f;    // Radius threshold
    float threshold_sq   = 1.0f;    // Pre-calculated squared threshold
    Comparison op        = Comparison::LESS;

    /**
     * execute_cull
     * Evaluates spatial proximity and prunes the selection.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            _cull_sparse(r_selection, p_view);
        }
    }

private:
    /**
     * _evaluate_comp
     * Templated comparison for distance results.
     */
    template <Comparison C>
    inline bool _evaluate_comp(float p_dist_sq, float p_thresh_sq) const {
        if constexpr (C == Comparison::EQUAL)         return p_dist_sq == p_thresh_sq;
        if constexpr (C == Comparison::NOT_EQUAL)     return p_dist_sq != p_thresh_sq;
        if constexpr (C == Comparison::LESS)          return p_dist_sq <  p_thresh_sq;
        if constexpr (C == Comparison::LESS_EQUAL)    return p_dist_sq <= p_thresh_sq;
        if constexpr (C == Comparison::GREATER)       return p_dist_sq >  p_thresh_sq;
        if constexpr (C == Comparison::GREATER_EQUAL) return p_dist_sq >= p_thresh_sq;
        return false;
    }

    /**
     * _evaluate
     * Core distance math. Uses squared distance to maximize throughput.
     */
    template <Comparison C>
    inline bool _evaluate(const T& p_val) const {
        float dist_sq = 0.0f;

        if constexpr (std::is_arithmetic_v<T>) {
            // 1D distance (Scalar)
            float diff = static_cast<float>(p_val) - static_cast<float>(target_center);
            dist_sq = diff * diff;
        } else {
            // N-Dimensional distance via Godot vector types
            // Godot vectors provide distance_squared_to which is highly optimized.
            dist_sq = p_val.distance_squared_to(target_center);
        }

        return _evaluate_comp<C>(dist_sq, threshold_sq);
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        switch (op) {
            case Comparison::EQUAL:         _loop_dense<Comparison::EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::NOT_EQUAL:     _loop_dense<Comparison::NOT_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::LESS:          _loop_dense<Comparison::LESS>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::LESS_EQUAL:    _loop_dense<Comparison::LESS_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::GREATER:       _loop_dense<Comparison::GREATER>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::GREATER_EQUAL: _loop_dense<Comparison::GREATER_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
        }
    }

    template <Comparison C, typename T_View>
    inline void _loop_dense(uint64_t* p_bitset, int64_t p_cap, const T_View& p_view, int64_t& r_count) const {
        for (int64_t i = 0; i < p_cap; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<C>(p_view[i])) {
                    p_bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        switch (op) {
            case Comparison::EQUAL:         _loop_sparse<Comparison::EQUAL>(indices, count, p_view, write_ptr); break;
            case Comparison::NOT_EQUAL:     _loop_sparse<Comparison::NOT_EQUAL>(indices, count, p_view, write_ptr); break;
            case Comparison::LESS:          _loop_sparse<Comparison::LESS>(indices, count, p_view, write_ptr); break;
            case Comparison::LESS_EQUAL:    _loop_sparse<Comparison::LESS_EQUAL>(indices, count, p_view, write_ptr); break;
            case Comparison::GREATER:       _loop_sparse<Comparison::GREATER>(indices, count, p_view, write_ptr); break;
            case Comparison::GREATER_EQUAL: _loop_sparse<Comparison::GREATER_EQUAL>(indices, count, p_view, write_ptr); break;
        }
        r_selection.element_count = write_ptr;
    }

    template <Comparison C, typename T_View>
    inline void _loop_sparse(int64_t* p_indices, int64_t p_count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            const int64_t idx = p_indices[i];
            if (_evaluate<C>(p_view[idx])) {
                p_indices[r_write_ptr++] = idx;
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DISTANCE_CULL_LOGIC_H