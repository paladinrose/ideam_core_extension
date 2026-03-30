#ifndef IDEAM_CORE_DATA_RANGE_CULL_LOGIC_H
#define IDEAM_CORE_DATA_RANGE_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <type_traits>

namespace ideam::core {

/**
 * DataRangeCullLogic<T>
 * Filters elements based on whether their value falls within [min, max].
 * T: The data type (float, int32_t, Vector3, etc.).
 * MAGIC: Uses direct typed comparisons to enable SIMD component-wise 
 * range checks for vector types.
 */
template <typename T>
struct DataRangeCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    enum class RangeMode : uint8_t {
        INCLUSIVE, // min <= val <= max
        EXCLUSIVE  // val < min OR val > max
    };

    // --- Configuration Data ---
    uint32_t column_id = 0;
    T range_min        = T{}; // Directly typed
    T range_max        = T{};
    RangeMode mode     = RangeMode::INCLUSIVE;

    /**
     * execute_cull
     * Evaluates the range and prunes the selection.
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
     * Core range logic. Optimized via 'inline bool'.
     */
    template <RangeMode M>
    inline bool _evaluate(const T& p_val) const {
        bool inside = false;

        // C++17/20 compile-time type detection
        if constexpr (std::is_arithmetic_v<T>) {
            // Scalar comparison (int, float, etc.)
            inside = (p_val >= range_min && p_val <= range_max);
        } else {
            // Vector/Composite comparison (Component-wise range check)
            // This replaces the legacy magnitude/radius check with a more 
            // performant AABB-style component-wise check.
            inside = (p_val >= range_min && p_val <= range_max);
        }

        return (M == RangeMode::INCLUSIVE) ? inside : !inside;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        if (mode == RangeMode::INCLUSIVE) {
            _loop_dense<RangeMode::INCLUSIVE>(bitset, count, p_view, r_selection.element_count);
        } else {
            _loop_dense<RangeMode::EXCLUSIVE>(bitset, count, p_view, r_selection.element_count);
        }
    }

    template <RangeMode M, typename T_View>
    inline void _loop_dense(uint64_t* p_bitset, int64_t p_cap, const T_View& p_view, int64_t& r_count) const {
        for (int64_t i = 0; i < p_cap; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<M>(p_view[i])) {
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

        if (mode == RangeMode::INCLUSIVE) {
            _loop_sparse<RangeMode::INCLUSIVE>(indices, count, p_view, write_ptr);
        } else {
            _loop_sparse<RangeMode::EXCLUSIVE>(indices, count, p_view, write_ptr);
        }
        r_selection.element_count = write_ptr;
    }

    template <RangeMode M, typename T_View>
    inline void _loop_sparse(int64_t* p_indices, int64_t p_count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            const int64_t idx = p_indices[i];
            if (_evaluate<M>(p_view[idx])) {
                p_indices[r_write_ptr++] = idx;
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DATA_RANGE_CULL_LOGIC_H