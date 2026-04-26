#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/view_traits.h"
#include "../i_native_task.h"

#include <concepts>
#include <type_traits>

namespace ideam::core {

struct TransformLogicValidator {
    static constexpr bool validate(
        ViewCapability logic_caps, 
        BufferLayoutType logic_layouts, 
        DataType logic_types,
        ViewCapability view_caps, 
        BufferLayoutType view_layouts,
        DataType view_types) 
    {
        // 1. Hardware/Capability Access Validation
        if ((logic_caps & view_caps) != logic_caps) return false;

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
 * IsTransformLogic Concept (C++20)
 * Enforces the strict DOD contract for mathematical or reduction transforms.
 */
template <typename T>
concept IsTransformLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::required_capabilities } -> std::convertible_to<ViewCapability>;
    { T::required_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::required_types } -> std::convertible_to<DataType>;
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