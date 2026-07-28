#pragma once

#include "memory_buffer_selection_pod.h"
#include <cstdint>
#include <cstring>
#include <bit>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace ideam::core {

/**
 * SelectionUtils
 * Low-level, stateless bit manipulation and synchronization for MemoryBufferSelectionPOD.
 * Strictly operates in-place to prevent heap allocations during the hot path.
 */
struct SelectionUtils {

    /**
     * get_popcount
     * Returns the number of active bits in a dense bitset array.
     */
    [[nodiscard]] static inline int64_t get_popcount(const uint64_t* p_bitset, int64_t p_capacity) noexcept {
        int64_t count = 0;
        const int64_t words = (p_capacity + 63) >> 6;
        for (int64_t i = 0; i < words; ++i) {
            count += std::popcount(p_bitset[i]);
        }
        return count;
    }

    /**
     * convert_to_dense
     * Transitions a selection from Sparse to Dense mode IN-PLACE.
     * WARNING: This overwrites the sparse indices memory with the bitset!
     */
    static void convert_to_dense(MemoryBufferSelectionPOD& r_selection) {
        if (r_selection.mode == SelectionMode::DENSE) return;

        // 1. Cache the sparse indices we need to read from
        int64_t* indices = r_selection.data.indices;
        const int64_t count = r_selection.element_count;

        // 2. We can safely overwrite the indices pointer with the bitset pointer 
        // because they point to the exact same raw memory union.
        // A bitset (N bits) is always smaller than an index array (N * 64 bits), 
        // so we will never overwrite data we haven't read yet if we iterate forward.
        uint64_t* bitset = reinterpret_cast<uint64_t*>(r_selection.data.indices);
        
        // Clear the memory block to 0 before setting bits
        const int64_t words = (r_selection.capacity + 63) >> 6;
        std::memset(bitset, 0, words * sizeof(uint64_t));

        // 3. Populate the bitset from the cached sparse indices
        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            bitset[idx >> 6] |= (1ULL << (idx & 63));
        }

        // 4. Update the POD state
        r_selection.data.bitset = bitset;
        r_selection.mode = SelectionMode::DENSE;
        
        // Note: The parallel metadata arrays (group_masks, partition_ids, etc.) 
        // do not need to move. They are already sized to max_capacity and 
        // map 1:1 with the EntityID / Dense Index!
    }

    /**
     * convert_to_sparse
     * Transitions a selection from Dense to Sparse mode IN-PLACE.
     * Rebuilds the packed index array from the active bits.
     */
    static void convert_to_sparse(MemoryBufferSelectionPOD& r_selection) {
        if (r_selection.mode == SelectionMode::SPARSE) return;

        uint64_t* bitset = r_selection.data.bitset;
        int64_t* indices = reinterpret_cast<int64_t*>(r_selection.data.bitset);
        
        int64_t write_ptr = 0;
        const int64_t capacity = r_selection.capacity;

        // Iterate through the bitset and pack the active indices.
        for (int64_t i = 0; i < capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                indices[write_ptr++] = i;
            }
        }

        r_selection.data.indices = indices;
        r_selection.element_count = write_ptr;
        r_selection.mode = SelectionMode::SPARSE;
    }
};

} // namespace ideam::core

 // IDEAM_CORE_SELECTION_UTILS_H