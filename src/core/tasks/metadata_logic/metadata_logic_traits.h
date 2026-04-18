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
    NONE                  = 0,
    REQUIRES_SPATIAL      = 1 << 0, 
    REQUIRES_SIMD         = 1 << 1  
};

constexpr MetadataRequirement operator|(MetadataRequirement a, MetadataRequirement b) noexcept {
    return static_cast<MetadataRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has_metadata_requirement(MetadataRequirement p_mask, MetadataRequirement p_req) noexcept {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_req)) != 0;
}

struct MetadataLogicValidator {
    static constexpr bool validate(MetadataRequirement requirements, BufferLayoutType supported_layouts, ViewCapability view_caps, BufferLayoutType buffer_layout) {
        if (has_metadata_requirement(requirements, MetadataRequirement::REQUIRES_SIMD) && 
            !has_capability(view_caps, ViewCapability::SIMD_ACCESS)) return false;

        if (has_metadata_requirement(requirements, MetadataRequirement::REQUIRES_SPATIAL) && 
            !has_capability(view_caps, ViewCapability::SPATIAL_ACCESS)) return false;

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

 // IDEAM_CORE_METADATA_LOGIC_TRAITS_H