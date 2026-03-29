#ifndef IDEAM_CORE_MEMORY_GRANT_POD_H
#define IDEAM_CORE_MEMORY_GRANT_POD_H

#include "memory_common.h"
#include "memory_buffer_selection_pod.h"
#include <cstdint>
#include <new>

// Hardware alignment fallback for compilers lacking C++17 hardware_destructive_interference_size
#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t GRANT_CACHE_LINE = std::hardware_destructive_interference_size;
#else
    constexpr size_t GRANT_CACHE_LINE = 64; 
#endif

namespace ideam::core {

/**
 * GrantPartPOD
 * Represents a secured, pre-resolved claim to a specific MemoryBuffer's data.
 * Size forced to exactly 120 bytes to pack perfectly into Grant cache lines.
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

    // --- Explicit Tail Padding ---
// 6 bytes of padding brings the struct to exactly 128 bytes (2 cache lines).
uint8_t reserved_padding[6] = {0};
};

static_assert(sizeof(GrantPartPOD) % 8 == 0, "GrantPartPOD is not properly padded!");
static_assert(sizeof(GrantPartPOD) == 128, "GrantPartPOD size altered from expected 128 bytes!");

/**
 * TMemoryGrant
 * A templated security and access token. Forced to hardware cache-line boundaries.
 * @tparam N The maximum number of buffer claims allowed in this grant.
 */
template <uint32_t N>
struct alignas(GRANT_CACHE_LINE) TMemoryGrant {
    // Large array block (8-byte aligned base)
    GrantPartPOD parts[N];

    // --- Global Manager State (8-byte members) ---
    uint64_t manager_version_at_issue = 0;
    const uint64_t* global_manager_version_ptr = nullptr;

    // --- Hardware Handles (8-byte members) ---
    uint64_t uniform_set_handle = 0;

    // --- 4-Byte & 1-Byte State ---
    uint32_t part_count = 0;
    bool active = false;

    // --- Explicit Tail Padding ---
    // Guarantees the entire struct scales cleanly into exact cache line multiples.
    uint8_t reserved_padding[35] = {0};

    /**
     * get_part
     */
    [[nodiscard]] inline const GrantPartPOD* get_part(uint32_t p_index) const {
        return (p_index < part_count) ? &parts[p_index] : nullptr;
    }

    /**
     * is_valid
     * Validates the grant against global state and individual part staleness.
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
 */
using MemoryGrantPOD      = TMemoryGrant<4>;  // 576 bytes (Exactly 9 cache lines)
using MemoryGrantHeavyPOD = TMemoryGrant<8>;  // 1088 bytes (Exactly 17 cache lines)

// Compile-Time Defenses: Lock the exact memory footprints
static_assert(sizeof(MemoryGrantPOD) == 576, "MemoryGrantPOD (Lite) broke 9-cache-line perfection!");
static_assert(sizeof(MemoryGrantHeavyPOD) == 1088, "MemoryGrantHeavyPOD broke 17-cache-line perfection!");

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_GRANT_POD_H