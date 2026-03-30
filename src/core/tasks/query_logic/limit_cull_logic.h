#ifndef IDEAM_CORE_LIMIT_CULL_LOGIC_H
#define IDEAM_CORE_LIMIT_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <algorithm>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace ideam::core {

/**
 * LimitCullLogic
 * Caps the number of active elements in a selection to a fixed limit.
 * MAGIC: Performs in-place truncation of selection metadata with zero 
 * allocation overhead.
 */
struct LimitCullLogic {
    // --- View Binding & Logic Traits ---
    // Since we don't read buffer data, we use dummy types to satisfy the concept.
    using ValueType       = uint8_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<uint8_t, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    // Operates on any layout because it only modifies the Selection metadata.
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY;

    // --- Configuration Data ---
    int64_t limit = 0;

    /**
     * execute_cull
     * The DOD entry point. Truncates the selection to the specified limit.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (limit < 0) return;
        
        // Case: Clear the entire selection
        if (limit == 0) {
            _clear_all(r_selection);
            return;
        }

        // If we are already under the limit, do nothing.
        if (r_selection.element_count <= limit) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection);
        } else {
            _cull_sparse(r_selection);
        }
    }

private:
    void _clear_all(MemoryBufferSelectionPOD& r_selection) const {
        if (r_selection.mode == SelectionMode::DENSE) {
            const int64_t word_count = (r_selection.capacity + 63) >> 6;
            std::memset(r_selection.data.bitset, 0, word_count * sizeof(uint64_t));
        }
        r_selection.element_count = 0;
    }

    void _cull_dense(MemoryBufferSelectionPOD& r_selection) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t word_count = (r_selection.capacity + 63) >> 6;
        int64_t found_count = 0;

        for (int64_t i = 0; i < word_count; ++i) {
            const uint64_t word = bitset[i];
            if (word == 0) continue;

            // Use CPU intrinsics for high-speed population counting
#if defined(_MSC_VER)
            const int64_t word_pop = static_cast<int64_t>(__popcnt64(word));
#else
            const int64_t word_pop = static_cast<int64_t>(__builtin_popcountll(word));
#endif

            // Check if the "cutoff" point falls within this 64-bit word
            if (found_count + word_pop >= limit) {
                int64_t bits_to_keep = limit - found_count;
                uint64_t new_word = 0;
                uint64_t temp_word = word;

                // Extract and keep only the necessary bits
                for (int b = 0; b < 64 && bits_to_keep > 0; ++b) {
                    if (temp_word & (1ULL << b)) {
                        new_word |= (1ULL << b);
                        bits_to_keep--;
                    }
                }
                
                bitset[i] = new_word;

                // Prune all bits in all subsequent words
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
        // Sparse Truncation: We don't need to move memory, we just lower the 
        // logical 'element_count'. Downstream tasks will ignore indices 
        // beyond this count.
        r_selection.element_count = limit;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_LIMIT_CULL_LOGIC_H