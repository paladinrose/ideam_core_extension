#ifndef IDEAM_CORE_BITMASK_CULL_LOGIC_H
#define IDEAM_CORE_BITMASK_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstdint>

namespace ideam::core {

/**
 * BitmaskCullLogic<T>
 * Performs high-speed bitwise filtering on integer properties.
 * T: The underlying integer type (e.g., int32_t, int64_t).
 */
template <typename T>
struct BitmaskCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // Pure linear evaluation; no spatial or SIMD requirements by default.
    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    enum BitOp : uint8_t {
        MATCH_ALL, // (value & mask) == mask
        MATCH_ANY, // (value & mask) != 0
        MATCH_NONE // (value & mask) == 0
    };

    // --- Configuration Data ---
    uint32_t column_id = 0;  // Replaces legacy "property_name" string
    uint32_t mask      = 0;
    BitOp op           = MATCH_ALL;

    /**
     * execute_cull
     * The primary entry point for the QueryTask. 
     * Iterates through the selection and prunes elements that fail the bitmask test.
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
     * Core bitwise logic. Templated by BitOp to allow the compiler to 
     * resolve the branching at the call site (via execute_cull dispatch).
     */
    template <BitOp O>
    [[nodiscard]] FORCE_INLINE bool _evaluate(T p_val) const {
        const uint32_t val = static_cast<uint32_t>(p_val);
        if constexpr (O == MATCH_ALL) {
            return (val & mask) == mask;
        } else if constexpr (O == MATCH_ANY) {
            return (val & mask) != 0;
        } else if constexpr (O == MATCH_NONE) {
            return (val & mask) == 0;
        }
        return false;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        // Dispatch to the specific loop to keep the inner evaluation branchless
        switch (op) {
            case MATCH_ALL:  _loop_dense<MATCH_ALL>(bitset, count, p_view, r_selection.element_count); break;
            case MATCH_ANY:  _loop_dense<MATCH_ANY>(bitset, count, p_view, r_selection.element_count); break;
            case MATCH_NONE: _loop_dense<MATCH_NONE>(bitset, count, p_view, r_selection.element_count); break;
        }
    }

    template <BitOp O, typename T_View>
    FORCE_INLINE void _loop_dense(uint64_t* p_bitset, int64_t p_capacity, const T_View& p_view, int64_t& r_element_count) const {
        for (int64_t i = 0; i < p_capacity; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<O>(p_view[i])) {
                    p_bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_element_count--;
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
            case MATCH_ALL:  _loop_sparse<MATCH_ALL>(indices, count, p_view, write_ptr); break;
            case MATCH_ANY:  _loop_sparse<MATCH_ANY>(indices, count, p_view, write_ptr); break;
            case MATCH_NONE: _loop_sparse<MATCH_NONE>(indices, count, p_view, write_ptr); break;
        }
        r_selection.element_count = write_ptr;
    }

    template <BitOp O, typename T_View>
    FORCE_INLINE void _loop_sparse(int64_t* p_indices, int64_t p_count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            int64_t idx = p_indices[i];
            if (_evaluate<O>(p_view[idx])) {
                p_indices[r_write_ptr++] = idx;
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BITMASK_CULL_LOGIC_H