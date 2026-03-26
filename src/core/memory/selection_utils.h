#ifndef IDEAM_CORE_SELECTION_UTILS_H
#define IDEAM_CORE_SELECTION_UTILS_H

#include "memory_buffer_selection.h"
#include <cstdint>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace ideam::core {

/**
 * SelectionUtils
 * Low-level bit manipulation and synchronization for MemoryBufferSelection.
 * Handles the "Dense <-> Sparse" conversion logic.
 */
struct SelectionUtils {

    /**
     * get_popcount
     * Returns the number of active bits in a dense bitset.
     */
    [[nodiscard]] static inline int64_t get_popcount(const std::vector<uint64_t>& p_bitset) {
        int64_t count = 0;
        for (uint64_t word : p_bitset) {
#if defined(_MSC_VER)
            count += __popcnt64(word);
#else
            count += __builtin_popcountll(word);
#endif
        }
        return count;
    }

    /**
     * sync_indices_from_bitset
     * Populates the sparse index list and parallel metadata arrays from a dense bitset.
     */
    static void sync_indices_from_bitset(MemoryBufferSelection& r_selection) {
        r_selection.indices.clear();
        
        const int64_t active_count = get_popcount(r_selection.bitset);
        r_selection.indices.reserve(active_count);

        // Prepare temporary metadata storage to maintain parallelism
        std::vector<uint32_t> next_masks;
        std::vector<int64_t> next_parts;
        std::vector<uint32_t> next_versions;
        std::vector<uint8_t> next_lods;

        next_masks.reserve(active_count);
        next_parts.reserve(active_count);
        next_versions.reserve(active_count);
        next_lods.reserve(active_count);

        for (size_t i = 0; i < r_selection.bitset.size(); ++i) {
            uint64_t word = r_selection.bitset[i];
            while (word > 0) {
                uint32_t bit;
#if defined(_MSC_VER)
                _BitScanForward64((unsigned long*)&bit, word);
#else
                bit = __builtin_ctzll(word);
#endif
                const int64_t idx = static_cast<int64_t>(i << 6) + bit;
                
                r_selection.indices.push_back(idx);
                
                // Mirror metadata from dense storage to sparse storage
                if (idx < static_cast<int64_t>(r_selection.group_masks.size())) {
                    next_masks.push_back(r_selection.group_masks[idx]);
                    next_parts.push_back(r_selection.partition_ids[idx]);
                    next_versions.push_back(r_selection.version_tags[idx]);
                    next_lods.push_back(r_selection.lod_levels[idx]);
                }

                word &= ~(1ULL << bit);
            }
        }

        r_selection.group_masks = std::move(next_masks);
        r_selection.partition_ids = std::move(next_parts);
        r_selection.version_tags = std::move(next_versions);
        r_selection.lod_levels = std::move(next_lods);
    }

    /**
     * sync_bitset_from_indices
     * Rebuilds the bitset based on the current sparse indices.
     */
    static void sync_bitset_from_indices(MemoryBufferSelection& r_selection) {
        const size_t words = (r_selection.capacity + 63) >> 6;
        if (r_selection.bitset.size() != words) {
            r_selection.bitset.assign(words, 0);
        } else {
            std::fill(r_selection.bitset.begin(), r_selection.bitset.end(), 0);
        }

        for (int64_t idx : r_selection.indices) {
            r_selection.bitset[idx >> 6] |= (1ULL << (idx & 63));
        }
    }

    /**
     * convert_to_dense
     * Transition a selection from Sparse to Dense mode, expanding metadata.
     */
    static void convert_to_dense(MemoryBufferSelection& r_selection) {
        if (r_selection.mode == SelectionMode::DENSE) return;

        std::vector<uint32_t> new_masks(r_selection.capacity, 0);
        std::vector<int64_t> new_parts(r_selection.capacity, -1);
        std::vector<uint32_t> new_versions(r_selection.capacity, 0);
        std::vector<uint8_t> new_lods(r_selection.capacity, 0);
        
        const size_t words = (r_selection.capacity + 63) >> 6;
        r_selection.bitset.assign(words, 0);

        for (size_t i = 0; i < r_selection.indices.size(); ++i) {
            const int64_t idx = r_selection.indices[i];
            r_selection.bitset[idx >> 6] |= (1ULL << (idx & 63));
            
            new_masks[idx] = r_selection.group_masks[i];
            new_parts[idx] = r_selection.partition_ids[i];
            new_versions[idx] = r_selection.version_tags[i];
            new_lods[idx] = r_selection.lod_levels[i];
        }

        r_selection.group_masks = std::move(new_masks);
        r_selection.partition_ids = std::move(new_parts);
        r_selection.version_tags = std::move(new_versions);
        r_selection.lod_levels = std::move(new_lods);
        
        r_selection.indices.clear();
        r_selection.mode = SelectionMode::DENSE;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SELECTION_UTILS_H