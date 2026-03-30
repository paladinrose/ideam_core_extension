#ifndef IDEAM_CORE_PREDICATE_CULL_LOGIC_H
#define IDEAM_CORE_PREDICATE_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <type_traits>

namespace ideam::core {

/**
 * PredicateCullLogic<T>
 * A high-resolution filter supporting arithmetic, component-wise, and bitwise logic.
 * T: The data type (float, int32_t, Vector3, etc.).
 * MAGIC: Uses SFINAE/compile-time traits to specialize logic for scalars vs vectors.
 */
template <typename T>
struct PredicateCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY;

    enum class Comparison : uint8_t { 
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
        BIT_AND, BIT_OR, BIT_XOR 
    };
    
    enum ComponentMask : uint8_t {
        NONE = 0,
        CH_0 = 1 << 0,
        CH_1 = 1 << 1,
        CH_2 = 1 << 2,
        CH_3 = 1 << 3,
        ALL  = 0xF
    };

    // --- Configuration Data ---
    uint32_t column_id = 0;
    T threshold        = T{}; 
    Comparison op      = Comparison::EQUAL;
    uint8_t mask       = ALL;

    /**
     * execute_cull
     * The DOD entry point.
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
     * Core predicate engine.
     */
    template <Comparison C>
    inline bool _evaluate(const T& p_val) const {
        if constexpr (std::is_arithmetic_v<T>) {
            // --- Scalar Path ---
            if constexpr (C == Comparison::EQUAL)         return p_val == threshold;
            if constexpr (C == Comparison::NOT_EQUAL)     return p_val != threshold;
            if constexpr (C == Comparison::LESS)          return p_val <  threshold;
            if constexpr (C == Comparison::LESS_EQUAL)    return p_val <= threshold;
            if constexpr (C == Comparison::GREATER)       return p_val >  threshold;
            if constexpr (C == Comparison::GREATER_EQUAL) return p_val >= threshold;
            
            // Bitwise (Only valid for integral types)
            if constexpr (std::is_integral_v<T>) {
                if constexpr (C == Comparison::BIT_AND)   return (static_cast<int64_t>(p_val) & static_cast<int64_t>(threshold)) != 0;
                if constexpr (C == Comparison::BIT_OR)    return (static_cast<int64_t>(p_val) | static_cast<int64_t>(threshold)) != 0;
                if constexpr (C == Comparison::BIT_XOR)   return (static_cast<int64_t>(p_val) ^ static_cast<int64_t>(threshold)) != 0;
            }
        } else {
            // --- Vector/Composite Path (Component-Wise) ---
            const float* v_val = reinterpret_cast<const float*>(&p_val);
            const float* v_thr = reinterpret_cast<const float*>(&threshold);
            
            // Determine dimension based on sizeof(T)
            constexpr int dims = sizeof(T) / sizeof(float);

            for (int i = 0; i < dims; ++i) {
                if (mask & (1 << i)) {
                    bool satisfied = false;
                    if constexpr (C == Comparison::EQUAL)         satisfied = (v_val[i] == v_thr[i]);
                    else if constexpr (C == Comparison::NOT_EQUAL) satisfied = (v_val[i] != v_thr[i]);
                    else if constexpr (C == Comparison::LESS)          satisfied = (v_val[i] <  v_thr[i]);
                    else if constexpr (C == Comparison::LESS_EQUAL)    satisfied = (v_val[i] <= v_thr[i]);
                    else if constexpr (C == Comparison::GREATER)       satisfied = (v_val[i] >  v_thr[i]);
                    else if constexpr (C == Comparison::GREATER_EQUAL) satisfied = (v_val[i] >= v_thr[i]);

                    if (!satisfied) return false;
                }
            }
            return true;
        }
        return false;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        // Dispatch based on Comparison op to ensure branchless inner loops.
        switch (op) {
            case Comparison::EQUAL:         _loop_dense<Comparison::EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::NOT_EQUAL:     _loop_dense<Comparison::NOT_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::LESS:          _loop_dense<Comparison::LESS>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::LESS_EQUAL:    _loop_dense<Comparison::LESS_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::GREATER:       _loop_dense<Comparison::GREATER>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::GREATER_EQUAL: _loop_dense<Comparison::GREATER_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::BIT_AND:       _loop_dense<Comparison::BIT_AND>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::BIT_OR:        _loop_dense<Comparison::BIT_OR>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::BIT_XOR:       _loop_dense<Comparison::BIT_XOR>(bitset, count, p_view, r_selection.element_count); break;
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
            case Comparison::BIT_AND:       _loop_sparse<Comparison::BIT_AND>(indices, count, p_view, write_ptr); break;
            case Comparison::BIT_OR:        _loop_sparse<Comparison::BIT_OR>(indices, count, p_view, write_ptr); break;
            case Comparison::BIT_XOR:       _loop_sparse<Comparison::BIT_XOR>(indices, count, p_view, write_ptr); break;
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

#endif // IDEAM_CORE_PREDICATE_CULL_LOGIC_H