#ifndef IDEAM_CORE_MEMORY_GRANT_POD_H
#define IDEAM_CORE_MEMORY_GRANT_POD_H

#include "memory_common.h"
#include "memory_buffer_selection_pod.h"
#include <cstdint>

namespace ideam::core {

/**
 * GrantPartPOD
 * Represents a secured, pre-resolved claim to a specific MemoryBuffer's data.
 * The 'Selection' member defines WHICH elements within that buffer are accessible.
 */
struct GrantPartPOD {
    // --- 8-Byte Alignment Block ---
    uint8_t* raw_base_ptr = nullptr;     // Absolute start (Master + Offset)
    MemoryBufferSelectionPOD selection;   // Unified DOD Selection (88 bytes)

    // --- 4-Byte Alignment Block ---
    uint32_t buffer_id = 0xFFFFFFFF;
    uint32_t buffer_version_at_issue = 0;
    uint32_t element_stride = 0;         // Size of element (AoS)
    uint32_t column_id = 0;              // Target column (SoA)

    // --- 1-Byte Alignment Block ---
    BufferAccessMode access_mode = BufferAccessMode::READ;
    bool is_contiguous = false;          // SelectionMode::RANGE optimization flag
};

/**
 * TMemoryGrant
 * A templated security and access token.
 * @tparam N The maximum number of buffer claims allowed in this grant.
 */
template <uint32_t N>
struct TMemoryGrant {
    // Large array block (8-byte aligned)
    GrantPartPOD parts[N];

    // --- Global Manager State (8-byte members) ---
    uint64_t manager_version_at_issue = 0;
    const uint64_t* global_manager_version_ptr = nullptr;

    // --- Hardware Handles (8-byte members) ---
    uint64_t uniform_set_handle = 0;

    // --- 4-Byte & 1-Byte State ---
    uint32_t part_count = 0;
    bool active = false;

    /**
     * get_part
     */
    [[nodiscard]] inline const GrantPartPOD* get_part(uint32_t p_index) const {
        return (p_index < part_count) ? &parts[p_index] : nullptr;
    }

    /**
     * is_valid
     * Validates the grant against global state and individual part staleness.
     * The compiler can unroll this loop based on N for high performance.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool is_valid() const {
        if (!active) return false;

        // 1. Structural Invalidation (Global rebase check)
        if (global_manager_version_ptr && *global_manager_version_ptr != manager_version_at_issue) {
            return false;
        }

        // 2. Domain Invalidation (Per-part staleness check)
        for (uint32_t i = 0; i < part_count; ++i) {
            const GrantPartPOD& part = parts[i];
            
            if (!part.selection.is_valid() || 
                part.selection.manager_version != manager_version_at_issue ||
                part.buffer_id == 0xFFFFFFFF || 
                part.raw_base_ptr == nullptr) {
                return false;
            }
        }
        return true;
    }
};

/**
 * Standard Aliases
 * Use MemoryGrantPOD (Lite) for standard simulation nodes to maximize cache density.
 * Use MemoryGrantHeavyPOD for complex world-state resolvers.
 */
using MemoryGrantPOD      = TMemoryGrant<4>;  // ~504 bytes
using MemoryGrantHeavyPOD = TMemoryGrant<8>;  // ~984 bytes

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_GRANT_POD_H