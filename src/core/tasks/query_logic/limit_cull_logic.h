#ifndef IDEAM_CORE_LIMIT_CULL_LOGIC_H
#define IDEAM_CORE_LIMIT_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <algorithm>
#include <cstring>
#include <bit>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace ideam::core {

/**
 * LimitCullLogic
 * Caps the number of active elements in a selection to a fixed limit.
 * MAGIC: Add mode acts as an exact-count allocator by scanning the Availability Mask.
 */
struct LimitCullLogic {
    using ValueType       = uint8_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<uint8_t, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;
    int64_t limit = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) const {
        
        if (limit <= 0) {
            if constexpr (Op == QueryOp::CULL) r_selection.element_count = 0;
            return;
        }

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.element_count <= limit) return;
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection);
            else _cull_sparse(r_selection);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_limit(r_selection, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }

private:
    void _cull_dense(MemoryBufferSelectionPOD& r_selection) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t word_count = (r_selection.capacity + 63) >> 6;
        int64_t found_count = 0;

        for (int64_t i = 0; i < word_count; ++i) {
            uint64_t word = bitset[i];
            if (word == 0) continue;

#if defined(_MSC_VER)
            const int64_t word_pop = static_cast<int64_t>(__popcnt64(word));
#else
            const int64_t word_pop = static_cast<int64_t>(__builtin_popcountll(word));
#endif

            if (found_count + word_pop >= limit) {
                int64_t bits_to_keep = limit - found_count;
                uint64_t new_word = 0;
                uint64_t temp_word = word;

                for (int b = 0; b < 64 && bits_to_keep > 0; ++b) {
                    if (temp_word & (1ULL << b)) {
                        new_word |= (1ULL << b);
                        bits_to_keep--;
                    }
                }
                
                bitset[i] = new_word;

                if (i + 1 < word_count) {
                    std::memset(bitset + i + 1, 0, (word_count - (i + 1)) * sizeof(uint64_t));
                }
                
                r_selection.element_count = limit;
                return;
            }

            found_count += word_pop;
        }
    }

    void _cull_sparse(MemoryBufferSelectionPOD& r_selection) const {
        // Sparse Truncation is simply reducing the element count boundary.
        r_selection.element_count = limit;
    }

    void _add_limit(const MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        int64_t added_count = 0;
        const int64_t words = (r_selection.capacity + 63) >> 6;

        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0 && added_count < limit) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                p_ctx.queue_selection_command(target_buffer_id, global_index);
                added_count++;
                
                mask &= (mask - 1); 
            }
            if (added_count >= limit) break;
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_LIMIT_CULL_LOGIC_H