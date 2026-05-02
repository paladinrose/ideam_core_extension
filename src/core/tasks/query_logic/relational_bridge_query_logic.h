#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

/**
 * RelationalBridgeQueryLogic<T_Index>
 * Bridges two buffers using a direct index lookup column.
 * e.g., Cull the Mass buffer (Target) based on the Entity buffer (Source).
 * REQUIREMENT: source_selection MUST be in DENSE mode for O(1) bitset lookups.
 */
template <typename T_Index = int32_t>
struct RelationalBridgeQueryLogic {
    using ValueType       = T_Index; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T_Index, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::INT64;
    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* source_selection = nullptr;
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    static godot::Array get_ui_properties() {
        return godot::Array();
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!source_selection || source_selection->mode != SelectionMode::DENSE) return;

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

private:
    // --- The DOD View Adapter ---
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T_Index _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T_Index*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T_Index>(p_view[idx]); }) {
            return static_cast<T_Index>(p_view[idx]);
        } else {
            return T_Index{}; 
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(int64_t index, const T_View& p_view) const {
        // CORRECTED: Safely extract the relational index
        T_Index source_idx = _read_view(p_view, index); 
        
        if (source_idx < 0 || source_idx >= source_selection->capacity) return false;
        
        return (source_selection->data.bitset[source_idx >> 6] & (1ULL << (source_idx & 63))) != 0;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(i, p_view)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t write_ptr = 0;
        int64_t* indices = r_selection.data.indices;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (_evaluate(indices[i], p_view)) indices[write_ptr++] = indices[i];
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
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

                if (_evaluate(global_index, p_view)) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_RELATIONAL_BRIDGE_QUERY_LOGIC_H