#ifndef IDEAM_CORE_PAGED_TO_TILED_BRIDGE_QUERY_LOGIC_H
#define IDEAM_CORE_PAGED_TO_TILED_BRIDGE_QUERY_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/paged_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

/**
 * PagedToTiledBridgeQueryLogic
 * Target: TILED_SOA (Micro-chunks). Source: PAGED (Macro-world).
 * Maps active pages down to active processing tiles.
 */
struct PagedToTiledBridgeQueryLogic {
    using ValueType       = uint8_t; // Dummy type, operates on metadata
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = PagedView<uint8_t, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::TILED_SOA;
    static constexpr DataType supported_types = DataType::ANY;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* source_selection = nullptr; 
    uint32_t target_buffer_id = 0;
    
    // The ratio of TILED_SOA elements per PAGED table entry
    int64_t elements_per_page = 1; 

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!source_selection || source_selection->mode != SelectionMode::DENSE) return;

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_context);
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _is_page_active(int64_t target_idx) const {
        int64_t page_idx = target_idx / elements_per_page;
        if (page_idx >= source_selection->capacity) return false;
        return (source_selection->data.bitset[page_idx >> 6] & (1ULL << (page_idx & 63))) != 0;
    }

    void _cull_dense(MemoryBufferSelectionPOD& r_selection) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_is_page_active(i)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    void _add_available(const MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_is_page_active(global_index)) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_PAGED_TO_TILED_BRIDGE_QUERY_LOGIC_H