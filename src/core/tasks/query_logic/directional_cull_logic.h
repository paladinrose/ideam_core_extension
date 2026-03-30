#ifndef IDEAM_CORE_DIRECTIONAL_CULL_LOGIC_H
#define IDEAM_CORE_DIRECTIONAL_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <godot_cpp/variant/vector3.hpp>
#include <cmath>

namespace ideam::core {

/**
 * DirectionalCullLogic<T>
 * Filters elements based on their orientation relative to a target vector.
 * T: The vector type (Vector2, Vector3, Vector2i, Vector3i).
 */
template <typename T>
struct DirectionalCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    enum class Comparison : uint8_t {
        WITHIN_ANGLE,      // Dot Product >= cos(angle)
        OUTSIDE_ANGLE,     // Dot Product <  cos(angle)
        FACING_EACH_OTHER, // Dot Product <= -0.707
        PERPENDICULAR      // abs(Dot Product) < 0.1
    };

    // --- Configuration Data ---
    uint32_t column_id      = 0;
    T target_direction      = T{}; // Normalized direction vector
    float cos_threshold     = 0.707f; // Pre-calculated cos(angle)
    Comparison op           = Comparison::WITHIN_ANGLE;

    /**
     * execute_cull
     * Evaluates directional alignment and prunes the selection.
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
     * _evaluate
     * Core dot-product logic. Uses 'inline bool' for performance.
     */
    template <Comparison O>
    inline bool _evaluate(const T& p_val) const {
        // Godot vector types provide high-performance dot product overloads
        const float dot = p_val.dot(target_direction);

        if constexpr (O == Comparison::WITHIN_ANGLE)      return dot >= cos_threshold;
        if constexpr (O == Comparison::OUTSIDE_ANGLE)     return dot < cos_threshold;
        if constexpr (O == Comparison::FACING_EACH_OTHER) return dot <= -0.707f;
        if constexpr (O == Comparison::PERPENDICULAR)     return std::abs(dot) < 0.1f;
        
        return false;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        // Optimized dispatch to ensure the comparison branch is outside the hot loop
        switch (op) {
            case Comparison::WITHIN_ANGLE:      _loop_dense<Comparison::WITHIN_ANGLE>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::OUTSIDE_ANGLE:     _loop_dense<Comparison::OUTSIDE_ANGLE>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::FACING_EACH_OTHER: _loop_dense<Comparison::FACING_EACH_OTHER>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::PERPENDICULAR:     _loop_dense<Comparison::PERPENDICULAR>(bitset, count, p_view, r_selection.element_count); break;
        }
    }

    template <Comparison O, typename T_View>
    inline void _loop_dense(uint64_t* p_bitset, int64_t p_cap, const T_View& p_view, int64_t& r_count) const {
        for (int64_t i = 0; i < p_cap; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<O>(p_view[i])) {
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
            case Comparison::WITHIN_ANGLE:      _loop_sparse<Comparison::WITHIN_ANGLE>(indices, count, p_view, write_ptr); break;
            case Comparison::OUTSIDE_ANGLE:     _loop_sparse<Comparison::OUTSIDE_ANGLE>(indices, count, p_view, write_ptr); break;
            case Comparison::FACING_EACH_OTHER: _loop_sparse<Comparison::FACING_EACH_OTHER>(indices, count, p_view, write_ptr); break;
            case Comparison::PERPENDICULAR:     _loop_sparse<Comparison::PERPENDICULAR>(indices, count, p_view, write_ptr); break;
        }
        r_selection.element_count = write_ptr;
    }

    template <Comparison O, typename T_View>
    inline void _loop_sparse(int64_t* p_indices, int64_t p_count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            const int64_t idx = p_indices[i];
            if (_evaluate<O>(p_view[idx])) {
                p_indices[r_write_ptr++] = idx;
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DIRECTIONAL_CULL_LOGIC_H