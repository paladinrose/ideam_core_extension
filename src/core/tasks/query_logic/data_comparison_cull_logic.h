#ifndef IDEAM_CORE_DATA_COMPARISON_CULL_LOGIC_H
#define IDEAM_CORE_DATA_COMPARISON_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

namespace ideam::core {

/**
 * DataComparisonCullLogic<T>
 * Compares two properties against each other (e.g., A > B) for every element.
 * T: The data type being compared (int32, float, Vector3, etc).
 */
template <typename T>
struct DataComparisonCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    // The primary view is for Property A
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    enum class Operator : uint8_t {
        EQUAL,
        NOT_EQUAL,
        LESS_THAN,
        LESS_EQUAL,
        GREATER_THAN,
        GREATER_EQUAL
    };

    // --- Configuration Data ---
    uint32_t column_a_id = 0; 
    uint32_t column_b_id = 0;
    Operator op          = Operator::EQUAL;

    /**
     * execute_cull
     * Compares two columns. Because this requires two views, we resolve 
     * the second view (Property B) from the task context.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view_a, const TaskContextPOD& p_context) const {
        // Resolve the second view for Property B using the second GrantPart
        // We assume the QueryTask has baked column_b into part 1 of the grant.
        T_View view_b = p_context.get_view_for_part<T_View>(1);

        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view_a, view_b);
        } else {
            _cull_sparse(r_selection, p_view_a, view_b);
        }
    }

private:
    /**
     * _evaluate
     * Core comparison logic. Uses 'inline bool' as requested to clear compiler errors.
     */
    template <Operator O>
    inline bool _evaluate(const T& p_a, const T& p_b) const {
        if constexpr (O == Operator::EQUAL)         return p_a == p_b;
        if constexpr (O == Operator::NOT_EQUAL)     return p_a != p_b;
        if constexpr (O == Operator::LESS_THAN)      return p_a < p_b;
        if constexpr (O == Operator::LESS_EQUAL)     return p_a <= p_b;
        if constexpr (O == Operator::GREATER_THAN)   return p_a > p_b;
        if constexpr (O == Operator::GREATER_EQUAL)  return p_a >= p_b;
        return false;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view_a, const T_View& p_view_b) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        // Dispatch based on Operator to keep the inner loop branchless
        switch (op) {
            case Operator::EQUAL:         _loop_dense<Operator::EQUAL>(bitset, count, p_view_a, p_view_b, r_selection.element_count); break;
            case Operator::NOT_EQUAL:     _loop_dense<Operator::NOT_EQUAL>(bitset, count, p_view_a, p_view_b, r_selection.element_count); break;
            case Operator::LESS_THAN:      _loop_dense<Operator::LESS_THAN>(bitset, count, p_view_a, p_view_b, r_selection.element_count); break;
            case Operator::LESS_EQUAL:     _loop_dense<Operator::LESS_EQUAL>(bitset, count, p_view_a, p_view_b, r_selection.element_count); break;
            case Operator::GREATER_THAN:   _loop_dense<Operator::GREATER_THAN>(bitset, count, p_view_a, p_view_b, r_selection.element_count); break;
            case Operator::GREATER_EQUAL:  _loop_dense<Operator::GREATER_EQUAL>(bitset, count, p_view_a, p_view_b, r_selection.element_count); break;
        }
    }

    template <Operator O, typename T_View>
    inline void _loop_dense(uint64_t* p_bitset, int64_t p_cap, const T_View& p_view_a, const T_View& p_view_b, int64_t& r_count) const {
        for (int64_t i = 0; i < p_cap; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<O>(p_view_a[i], p_view_b[i])) {
                    p_bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view_a, const T_View& p_view_b) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        switch (op) {
            case Operator::EQUAL:         _loop_sparse<Operator::EQUAL>(indices, count, p_view_a, p_view_b, write_ptr); break;
            case Operator::NOT_EQUAL:     _loop_sparse<Operator::NOT_EQUAL>(indices, count, p_view_a, p_view_b, write_ptr); break;
            case Operator::LESS_THAN:      _loop_sparse<Operator::LESS_THAN>(indices, count, p_view_a, p_view_b, write_ptr); break;
            case Operator::LESS_EQUAL:     _loop_sparse<Operator::LESS_EQUAL>(indices, count, p_view_a, p_view_b, write_ptr); break;
            case Operator::GREATER_THAN:   _loop_sparse<Operator::GREATER_THAN>(indices, count, p_view_a, p_view_b, write_ptr); break;
            case Operator::GREATER_EQUAL:  _loop_sparse<Operator::GREATER_EQUAL>(indices, count, p_view_a, p_view_b, write_ptr); break;
        }
        r_selection.element_count = write_ptr;
    }

    template <Operator O, typename T_View>
    inline void _loop_sparse(int64_t* p_indices, int64_t p_count, const T_View& p_view_a, const T_View& p_view_b, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            const int64_t idx = p_indices[i];
            if (_evaluate<O>(p_view_a[idx], p_view_b[idx])) {
                p_indices[r_write_ptr++] = idx;
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DATA_COMPARISON_CULL_LOGIC_H