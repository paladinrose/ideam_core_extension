#ifndef IDEAM_CORE_TRANSFORM_LOGIC_TRAITS_H
#define IDEAM_CORE_TRANSFORM_LOGIC_TRAITS_H

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/view_traits.h"
#include "../i_native_task.h"

#include <concepts>
#include <type_traits>

namespace ideam::core {

/**
 * TransformRequirement
 * Bitmask defining the hardware or structural dependencies of a T_Logic payload.
 */
enum class TransformRequirement : uint32_t {
    NONE                  = 0,
    REQUIRES_SPATIAL      = 1 << 0, // Needs at(x,y,z) or neighbor stencils
    REQUIRES_SIMD         = 1 << 1, // Demands AOSOAView or get_lane()
    REQUIRES_ATOMIC       = 1 << 2, // Needs thread-safe aggregation (AtomicView)
    REQUIRES_PAGED        = 1 << 3  // Optimized for virtualized 4D memory
};

constexpr TransformRequirement operator|(TransformRequirement a, TransformRequirement b) noexcept {
    return static_cast<TransformRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has_transform_requirement(TransformRequirement p_mask, TransformRequirement p_req) noexcept {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_req)) != 0;
}

struct TransformLogicValidator {
    static constexpr bool validate(TransformRequirement requirements, BufferLayoutType supported_layouts, ViewCapability view_caps, BufferLayoutType buffer_layout) {
        
        if (has_transform_requirement(requirements, TransformRequirement::REQUIRES_SIMD) && 
            !has_capability(view_caps, ViewCapability::SIMD_ACCESS)) {
            return false;
        }

        if (has_transform_requirement(requirements, TransformRequirement::REQUIRES_SPATIAL) && 
            !has_capability(view_caps, ViewCapability::SPATIAL_ACCESS)) {
            return false;
        }

        return true;
    }
};

/**
 * IsTransformLogic Concept (C++20)
 * Enforces the strict DOD contract for mathematical or reduction transforms.
 */
template <typename T>
concept IsTransformLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::requirements } -> std::convertible_to<TransformRequirement>;
    { T::supported_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::supported_types } -> std::convertible_to<DataType>;
    { T::transient_workspace_bytes } -> std::convertible_to<size_t>;

    // Must implement templated execute_transform handling the injected Context and View
    { 
        std::declval<T>().template execute_transform<typename T::DefaultView, typename T::DefaultStrategy>(
            std::declval<const TaskContextPOD&>(), 
            std::declval<typename T::DefaultView&>()
        )
    } -> std::same_as<void>;
};

} // namespace ideam::core

#endif // IDEAM_CORE_TRANSFORM_LOGIC_TRAITS_H