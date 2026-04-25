#pragma once

#include "../memory_common.h" // Required for DataType bitmask
#include "../memory_buffer_pod.h" // Required for BufferLayoutType bitmask

#include <type_traits>
#include <cstdint>

namespace ideam::core {

/**
 * ViewCapability
 * Bitmask for high-level T_Logic dispatching.
 * Defines the API contract and methods exposed by the View.
 */
enum class ViewCapability : uint32_t {
    NONE                   = 0,
    LINEAR_ACCESS          = 1 << 0,  // Supports operator[] selection-relative
    SPATIAL_ACCESS         = 1 << 1,  // Supports multidimensional indexing
    SIMD_ACCESS            = 1 << 2,  // Supports get_lane / LaneWidth
    RANDOM_ACCESS          = 1 << 3,  // Supports arbitrary ID lookups
    VIRTUAL_MEMORY         = 1 << 4,  // Paged/Indirect (Strategy-dependent)
    QUEUE_ACCESS           = 1 << 5,  // Supports stateful consumption (pop)
    STENCIL_ACCESS         = 1 << 6,  // Supports center() and neighbor(k)
    SWAP_ACCESS            = 1 << 7,  // Supports Temporal Ping-Pong read/write proxies
    ENTITY_ID_ACCESS       = 1 << 8,  // Supports get_entity_at() for Sparse ECS bridging
    MULTI_COMPONENT_ACCESS = 1 << 9,  // Supports pluck<T>() for heterogeneous payloads
    ATOMIC_ACCESS          = 1 << 10  // Supports std::atomic_ref returns
};

constexpr ViewCapability operator|(ViewCapability a, ViewCapability b) noexcept {
    return static_cast<ViewCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has_capability(ViewCapability p_mask, ViewCapability p_cap) noexcept {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_cap)) != 0;
}

/**
 * ViewStrategies
 * Bitmask defining which underlying MemoryStrategies a View can safely bind to
 * without causing cache thrashing or undefined behavior.
 */
enum class ViewStrategies : uint32_t {
    NONE       = 0,
    FLAT       = 1 << 0,
    SOA        = 1 << 1,
    AOS        = 1 << 2,
    SPATIAL_2D = 1 << 3,
    SPATIAL_3D = 1 << 4,
    SPATIAL_4D = 1 << 5,
    TILED_SOA  = 1 << 6,
    RING       = 1 << 7,
    PAGED      = 1 << 8,

    // --- Aggregation Masks ---
    ANY_SPATIAL = SPATIAL_2D | SPATIAL_3D | SPATIAL_4D,
    ANY_LINEAR  = FLAT | SOA | AOS | RING,
    ANY         = 0xFFFFFFFF
};

constexpr ViewStrategies operator|(ViewStrategies a, ViewStrategies b) noexcept {
    return static_cast<ViewStrategies>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr ViewStrategies operator&(ViewStrategies a, ViewStrategies b) noexcept {
    return static_cast<ViewStrategies>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * ViewTraits<View>
 * Standardized DOD metadata for all MemoryViews.
 * Allows the Job Graph to statically assert requirements before hardware execution.
 */
template<typename T_View>
struct ViewTraits {
   
    // --- API Interface Capabilities ---
    static constexpr ViewCapability capabilities = ViewCapability::NONE;

    // --- DOD Layout Restrictors ---
    static constexpr BufferLayoutType supported_layouts  = BufferLayoutType::NONE;
    static constexpr ViewStrategies supported_strategies = ViewStrategies::NONE;
    static constexpr DataType       supported_types      = DataType::NONE;

    // --- Hardware Alignment ---
    static constexpr uint32_t lane_width = 1;
};

} // namespace ideam::core