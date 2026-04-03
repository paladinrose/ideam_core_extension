#ifndef IDEAM_CORE_DATA_COMPARISON_CULL_LOGIC_H
#define IDEAM_CORE_DATA_COMPARISON_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

template <typename T>
struct DataComparisonCullLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    enum class Operator : uint8_t { EQUAL, NOT_EQUAL, LESS_THAN, LESS_EQUAL, GREATER_THAN, GREATER_EQUAL };

    uint32_t target_buffer_id = 0;
    uint32_t column_id_a = 0;
    
    // Config for the secondary view
    uint32_t comparison_buffer_id = 0;
    uint32_t column_id_b = 0;
    Operator op = Operator::EQUAL;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& view_a, 
                      const T_View& view_b) const { // Note: Dual view injection
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, view_a, view_b);
            else _cull_sparse(r_selection, view_a, view_b);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, view_a, view_b, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& val_a, const T& val_b) const {
        switch (op) {
            case Operator::EQUAL:         return val_a == val_b;
            case Operator::NOT_EQUAL:     return val_a != val_b;
            case Operator::LESS_THAN:     return val_a < val_b;
            case Operator::LESS_EQUAL:    return val_a <= val_b;
            case Operator::GREATER_THAN:  return val_a > val_b;
            case Operator::GREATER_EQUAL: return val_a >= val_b;
        }
        return false;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& view_a, const T_View& view_b) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(view_a[i], view_b[i])) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& view_a, const T_View& view_b) const {
        int64_t write_ptr = 0;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (_evaluate(view_a[i], view_b[i])) {
                r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& view_a, const T_View& view_b, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate(view_a[global_index], view_b[global_index])) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DATA_COMPARISON_CULL_LOGIC_H