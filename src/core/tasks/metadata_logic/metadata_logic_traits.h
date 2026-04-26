#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/view_traits.h"
#include "../i_native_task.h"

#include <concepts>
#include <type_traits>

namespace ideam::core {

struct MetadataLogicValidator {
    static constexpr bool validate(
        ViewCapability logic_caps, 
        BufferLayoutType logic_layouts, 
        DataType logic_types,
        ViewCapability view_caps, 
        BufferLayoutType view_layouts,
        DataType view_types) 
    {
        // 1. Hardware/Capability Access Validation (O(1) DOD Bitwise Fast Path)
        // Validates that the View possesses ALL the capabilities demanded by the Logic.
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
 * IsMetadataLogic Concept (C++20)
 * Enforces the DOD contract for Shadow Buffer manipulators.
 */
template <typename T>
concept IsMetadataLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::required_capabilities } -> std::convertible_to<ViewCapability>;
    { T::required_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::required_types } -> std::convertible_to<DataType>;
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