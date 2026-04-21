#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/sparse_set_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h" 
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

/**
 * ArchetypeQueryLogic
 * Filters a selection of entities based on the presence of required components.
 */
struct ArchetypeQueryLogic {
    using ValueType       = uint32_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SparseSetView<ValueType, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::SPARSE_SET;
    static constexpr DataType supported_types = DataType::INT32;
    
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr size_t MAX_SIGNATURE_SIZE = 16;
    
    uint32_t target_buffer_id = 0; 
    uint32_t required_buffer_ids[MAX_SIGNATURE_SIZE] = {0};
    uint32_t required_count = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template<QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view, p_context);
            else _cull_sparse(r_selection, p_view, p_context);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

private:

    // --- The DOD View Adapter (Archetype / uint32_t Specialized) ---
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline uint32_t _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const uint32_t*>(p_view[idx]);
        } else if constexpr (requires { static_cast<uint32_t>(p_view[idx]); }) {
            return static_cast<uint32_t>(p_view[idx]);
        } else {
            return 0; 
        }
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(uint32_t p_entity_id, const TaskContextPOD& p_context) const {
        for (uint32_t i = 0; i < required_count; ++i) {
            if (!p_context.manager->buffer_contains_id(required_buffer_ids[i], p_entity_id)) {
                return false;
            }
        }
        return true;
    }

    template<typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(_read_view(p_view, i), p_ctx)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template<typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;
        for (int64_t i = 0; i < count; ++i) {
            if (_evaluate(_read_view(p_view, i), p_ctx)) {
                r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
            }
        }
        r_selection.element_count = write_ptr;
    }

    template<typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate(_read_view(p_view, global_index), p_ctx)) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_ARCHETYPE_QUERY_LOGIC_H