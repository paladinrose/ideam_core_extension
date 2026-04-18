#pragma once

#include "memory_common.h"
#include <cstdint>

namespace ideam::core {

/**
 * SelectionMode
 * Defines the mathematical interpretation of the selection data.
 */
enum class SelectionMode : uint8_t {
    SPARSE, // Data is an array of int64_t indices (element_count)
    DENSE,  // Data is a bitmask (capacity / 64 words)
    RANGE   // Data is a contiguous slice [start_index, element_count]
};

/**
 * MemoryBufferSelectionPOD
 * A synthesized DOD handle. Represents a filtered subset of a MemoryBuffer.
 * Designed for SIMD-aligned bitset collision and Reactive Graph propagation.
 * Fixed at exactly 104 bytes to ensure parent Grants align to CPU cache lines.
 */
struct MemoryBufferSelectionPOD {
    // --- 8-Byte Alignment Block (Pointers & 64-bit Ints) ---
    // Total: 88 bytes. Grouped to prevent internal structure fragmentation.
    uint64_t buffer_version = 0;     // Snapshot of BufferPOD.version
    uint64_t manager_version = 0;    // Snapshot of Manager.version (detects rebases)
    uint64_t selection_version = 0;  // Self-version for downstream memoization

    int64_t element_count = 0; // Number of items selected
    int64_t capacity = 0;      // Range of the target buffer (max index)
    int64_t start_index = 0;   // Used primarily for RANGE mode

    // --- Data Pointers (Points to Manager-controlled memory) ---
    // These pointers are volatile; validity is tied to manager_version.
    union {
        void* raw_data;
        int64_t* indices;   // Valid if mode == SPARSE
        uint64_t* bitset;   // Valid if mode == DENSE (SIMD padded)
    } data = {nullptr};

    // --- Metadata SoA (Parallel Streams) ---
    // All pointers are 8 bytes on x64; grouping them maintains cache locality.
    int64_t* partition_ids  = nullptr;
    uint32_t* group_masks   = nullptr;
    uint32_t* version_tags  = nullptr;
    uint8_t* lod_levels     = nullptr;
    uint64_t* unclaimed_mask = nullptr; // Availability Mask (Anti-Grant Bitset)

    // --- 4-Byte Alignment Block ---
    // Total: 4 bytes.
    uint32_t target_buffer_id = 0;   // The buffer this selection targets

    // --- 1-Byte Alignment Block ---
    // Total: 2 bytes. 
    SelectionMode mode = SelectionMode::SPARSE;
    BufferAlignmentMode alignment = BufferAlignmentMode::STD430;

    // --- Explicit Tail Padding ---
    // Replaces 2 bytes of implicit compiler padding to cap the struct at exactly 104 bytes.
    uint8_t reserved_padding[2] = {0};

    /**
     * is_selected
     * Critical path logic for View-based filtering.
     */
    [[nodiscard]] inline bool is_selected(int64_t p_index) const {
        if (p_index < 0 || p_index >= capacity) return false;

        switch (mode) {
            case SelectionMode::DENSE: {
                return (data.bitset[p_index >> 6] & (1ULL << (p_index & 63))) != 0;
            }
            case SelectionMode::RANGE: {
                return p_index >= start_index && p_index < (start_index + element_count);
            }
            case SelectionMode::SPARSE: {
                // O(N) search: Not recommended for hot-path 'is_selected' checks.
                // Use a SparseSetView for linear iteration instead.
                return false; 
            }
        }
        return false;
    }

    /**
     * is_valid
     * Verifies that the selection isn't logically or physically dangling.
     */
    [[nodiscard]] constexpr bool is_valid() const {
        return (mode == SelectionMode::RANGE || data.raw_data != nullptr) && target_buffer_id != 0;
    }
};

// Compile-Time Defenses: Lock the exact memory footprints
static_assert(sizeof(MemoryBufferSelectionPOD) % 8 == 0, "MemoryBufferSelectionPOD alignment violated!");
static_assert(sizeof(MemoryBufferSelectionPOD) == 104, "MemoryBufferSelectionPOD size altered from expected 104 bytes!");

} // namespace ideam::core

 // IDEAM_CORE_MEMORY_BUFFER_SELECTION_POD_H