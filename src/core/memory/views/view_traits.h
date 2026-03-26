#ifndef IDEAM_CORE_VIEW_TRAITS_H
#define IDEAM_CORE_VIEW_TRAITS_H

#include <type_traits>
#include <cstdint>

namespace ideam::core {

/**
 * ViewCapabilities
 * Bitmask for high-level T_Logic dispatching.
 */
enum class ViewCapability : uint32_t {
    NONE            = 0,
    LINEAR_ACCESS   = 1 << 0, // Supports operator[] selection-relative
    SPATIAL_ACCESS  = 1 << 1, // Supports at(x, y, z)
    SIMD_ACCESS     = 1 << 2, // Supports get_lane / LaneWidth
    RANDOM_ACCESS   = 1 << 3, // Supports arbitrary ID lookups
    VIRTUAL_MEMORY  = 1 << 4  // Paged/Indirect (Strategy-dependent)
};

inline constexpr ViewCapability operator|(ViewCapability a, ViewCapability b) {
    return static_cast<ViewCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr bool has_capability(ViewCapability p_mask, ViewCapability p_cap) {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_cap)) != 0;
}

/**
 * ViewTraits<View>
 * Standardized metadata for all MemoryViews.
 * Allows T_Logic to statically assert requirements (e.g., must be SPATIAL_ACCESS).
 */
template<typename T_View>
struct ViewTraits {
    // These defaults should be overridden by specialized View implementations
    static constexpr ViewCapability capabilities = ViewCapability::NONE;
    static constexpr bool is_spatial = false;
    static constexpr bool is_simd = false;
    static constexpr uint32_t lane_width = 1;
};

} // namespace ideam::core

#endif // IDEAM_CORE_VIEW_TRAITS_H