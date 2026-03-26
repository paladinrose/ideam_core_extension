#ifndef IDEAM_CORE_MEMORY_BUFFER_SELECTION_H
#define IDEAM_CORE_MEMORY_BUFFER_SELECTION_H

#include "memory_common.h"
#include <vector>
#include <cstdint>
#include <algorithm>

namespace ideam::core {

enum class SelectionMode : int64_t {
    SPARSE,
    DENSE
};

/**
 * MemoryBufferSelection
 * Manages targeted elements for DOD operations.
 * Uses MemoryUtilities to align bitsets for SIMD/GPU compatibility.
 */
struct MemoryBufferSelection {
    SelectionMode mode = SelectionMode::SPARSE;
    BufferAlignmentMode alignment_mode = BufferAlignmentMode::STD430;
    
    int64_t capacity = 0;
    uint32_t buffer_id = 0;
    uint64_t version = 0;

    // --- The Selection Sets ---
    std::vector<int64_t> indices;   // Sparse list
    std::vector<uint64_t> bitset;   // Dense bitmask (Type: INT64)

    // --- Metadata Primitives (SoA) ---
    std::vector<uint32_t> group_masks;
    std::vector<int64_t> partition_ids;
    std::vector<uint32_t> version_tags;
    std::vector<uint8_t> lod_levels;

    /**
     * resize
     * Coordinates the bitset footprint with MemoryUtilities alignment rules.
     */
    void resize(int64_t p_element_count) {
        capacity = p_element_count;
        
        if (mode == SelectionMode::DENSE) {
            // 1. Determine the raw requirement for a 64-bit word in the current mode.
            const size_t word_alignment = MemoryUtilities::get_type_alignment(DataType::INT64, alignment_mode);
            
            // 2. Calculate initial word count (1 bit per element).
            size_t words_needed = (p_element_count + 63) / 64;

            // 3. AVX2/SIMD Padding:
            // CollisionUtils processes 4 words (256 bits) per iteration.
            // We must align the total count to a 4-word boundary to prevent out-of-bounds reads.
            const size_t simd_word_boundary = 4; 
            size_t padded_words = (words_needed + (simd_word_boundary - 1)) & ~(simd_word_boundary - 1);

            // 4. Apply the MemoryUtilities alignment to the total byte size if necessary.
            // (Standard vectors handle their internal pointer, but we control the 'count' here).
            bitset.assign(padded_words, 0);
            
            // Metadata follows 1:1 element count.
            group_masks.resize(p_element_count, 0);
            partition_ids.resize(p_element_count, -1);
            version_tags.resize(p_element_count, 0);
            lod_levels.resize(p_element_count, 0);
        }
        version++;
    }

    void clear() {
        if (mode == SelectionMode::DENSE) {
            std::fill(bitset.begin(), bitset.end(), 0);
            std::fill(group_masks.begin(), group_masks.end(), 0);
            std::fill(partition_ids.begin(), partition_ids.end(), -1);
            std::fill(version_tags.begin(), version_tags.end(), 0);
            std::fill(lod_levels.begin(), lod_levels.end(), 0);
        } else {
            indices.clear();
            group_masks.clear();
            partition_ids.clear();
            version_tags.clear();
            lod_levels.clear();
        }
        version++;
    }

    [[nodiscard]] bool is_empty() const {
        if (mode == SelectionMode::SPARSE) return indices.empty();
        for (uint64_t word : bitset) {
            if (word != 0) return false;
        }
        return true;
    }

    /**
     * set_index
     * Individual bit mutation.
     */
    inline void set_index(int64_t p_idx, bool p_active) {
        if (p_idx < 0 || p_idx >= capacity) return;
        
        uint64_t& word = bitset[p_idx >> 6];
        uint64_t mask = 1ULL << (p_idx & 63);
        if (p_active) word |= mask;
        else word &= ~mask;
        
        version++;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_BUFFER_SELECTION_H