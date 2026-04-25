#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/view_traits.h"
#include "../i_native_task.h"

#include <concepts>
#include <type_traits>

namespace ideam::core {

/**
 * MetadataRequirement
 * Bitmask defining the structural/hardware dependencies of a metadata logic struct.
 */
enum class MetadataRequirement : uint32_t {
    NONE                     = 0,
    REQUIRES_SPATIAL         = 1 << 0, 
    REQUIRES_SIMD            = 1 << 1,
    REQUIRES_ATOMIC          = 1 << 2, 
    REQUIRES_PAGED           = 1 << 3, 
    READ_ONLY_DATA           = 1 << 4,
    REQUIRES_QUEUE           = 1 << 5, 
    REQUIRES_STENCIL         = 1 << 6,
    REQUIRES_SWAP            = 1 << 7,
    REQUIRES_ENTITY_ID       = 1 << 8,
    REQUIRES_MULTI_COMPONENT = 1 << 9
};

constexpr MetadataRequirement operator|(MetadataRequirement a, MetadataRequirement b) noexcept {
    return static_cast<MetadataRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has_metadata_requirement(MetadataRequirement p_mask, MetadataRequirement p_req) noexcept {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_req)) != 0;
}

struct MetadataLogicValidator {
    static constexpr bool validate(
        MetadataRequirement logic_reqs, 
        BufferLayoutType logic_layouts, 
        DataType logic_types,
        ViewCapability view_caps, 
        BufferLayoutType view_layouts,
        DataType view_types) 
    {
        // 1. Hardware/Capability Access Validation
        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_SPATIAL) && 
            !has_capability(view_caps, ViewCapability::SPATIAL_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_SIMD) && 
            !has_capability(view_caps, ViewCapability::SIMD_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_ATOMIC) && 
            !has_capability(view_caps, ViewCapability::ATOMIC_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_QUEUE) && 
            !has_capability(view_caps, ViewCapability::QUEUE_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_STENCIL) && 
            !has_capability(view_caps, ViewCapability::STENCIL_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_SWAP) && 
            !has_capability(view_caps, ViewCapability::SWAP_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_ENTITY_ID) && 
            !has_capability(view_caps, ViewCapability::ENTITY_ID_ACCESS)) return false;

        if (has_metadata_requirement(logic_reqs, MetadataRequirement::REQUIRES_MULTI_COMPONENT) && 
            !has_capability(view_caps, ViewCapability::MULTI_COMPONENT_ACCESS)) return false;

        // 2. Structural Layout Intersection
        // Validates that the View supports at least one memory layout required by the Logic payload.
        if ((logic_layouts & view_layouts) == BufferLayoutType::NONE) return false;

        // 3. Payload DataType Intersection
        // Validates that the View supports at least one underlying primitive type that the Logic expects.
        if ((logic_types & view_types) == DataType::NONE) return false;

        return true;
    }
};

/**
 * IsMetadataLogic Concept (C++20)
 * Enforces the DOD contract for Shadow Buffer manipulators.
 */
template <typename T>
concept IsMetadataLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::requirements } -> std::convertible_to<MetadataRequirement>;
    { T::supported_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::supported_types } -> std::convertible_to<DataType>;
    { T::transient_workspace_bytes } -> std::convertible_to<size_t>;

    // Must implement templated execute_metadata handling the mutable selection
    { 
        std::declval<T>().template execute_metadata<typename T::DefaultView, typename T::DefaultStrategy>(
            std::declval<MemoryBufferSelectionPOD&>(),
            std::declval<const TaskContextPOD&>(), 
            std::declval<typename T::DefaultView&>()
        ) 
    } -> std::same_as<void>;
} && std::is_trivially_copyable_v<T>;

} // namespace ideam::core