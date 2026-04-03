#ifndef IDEAM_CORE_BITMASK_QUERY_LOGIC_H
#define IDEAM_CORE_BITMASK_QUERY_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstdint>
#include <bit>

namespace ideam::core {

/**
 * BitmaskQueryLogic<T>
 * Performs high-speed bitwise filtering on integer properties.
 */
template <typename T>
struct BitmaskQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    enum BitOp : uint8_t {
        MATCH_ALL, // (value & mask) == mask
        MATCH_ANY, // (value & mask) != 0
        MATCH_NONE // (value & mask) == 0
    };

    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0; 
    T mask = 0;
    BitOp op = MATCH_ALL;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

private:
    template <BitOp O>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(T p_val) const {
        if constexpr (O == MATCH_ALL)  return (p_val & mask) == mask;
        if constexpr (O == MATCH_ANY)  return (p_val & mask) != 0;
        if constexpr (O == MATCH_NONE) return (p_val & mask) == 0;
        return false;
    }

    template <BitOp O, typename T_View>
    void _loop_dense_cull(uint64_t* p_bitset, int64_t p_capacity, const T_View& p_view, int64_t& r_element_count) const {
        for (int64_t i = 0; i < p_capacity; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<O>(p_view[i])) {
                    p_bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_element_count--;
                }
            }
        }
    }

    template <BitOp O, typename T_View>
    void _loop_sparse_cull(int64_t* indices, int64_t p_count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            if (_evaluate<O>(p_view[i])) {
                indices[r_write_ptr++] = indices[i];
            }
        }
    }

    template <BitOp O, typename T_View>
    void _loop_add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t m = unclaimed[w];
            while (m != 0) {
                int bit_index = std::countr_zero(m);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate<O>(p_view[global_index])) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                m &= (m - 1); 
            }
        }
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        switch (op) {
            case MATCH_ALL:  _loop_dense_cull<MATCH_ALL>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case MATCH_ANY:  _loop_dense_cull<MATCH_ANY>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case MATCH_NONE: _loop_dense_cull<MATCH_NONE>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t write_ptr = 0;
        if (r_selection.mode == SelectionMode::SPARSE) {
            switch (op) {
                case MATCH_ALL:  _loop_sparse_cull<MATCH_ALL>(r_selection.data.indices, r_selection.element_count, p_view, write_ptr); break;
                case MATCH_ANY:  _loop_sparse_cull<MATCH_ANY>(r_selection.data.indices, r_selection.element_count, p_view, write_ptr); break;
                case MATCH_NONE: _loop_sparse_cull<MATCH_NONE>(r_selection.data.indices, r_selection.element_count, p_view, write_ptr); break;
            }
            r_selection.element_count = write_ptr;
        }
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        switch (op) {
            case MATCH_ALL:  _loop_add_available<MATCH_ALL>(r_selection, p_view, p_ctx); break;
            case MATCH_ANY:  _loop_add_available<MATCH_ANY>(r_selection, p_view, p_ctx); break;
            case MATCH_NONE: _loop_add_available<MATCH_NONE>(r_selection, p_view, p_ctx); break;
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BITMASK_QUERY_LOGIC_H