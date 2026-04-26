#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/view_traits.h"

#include "../i_native_task.h"

#include <concepts>
#include <type_traits>

namespace ideam::core {

/**
 * QueryOp
 * Compile-time router for T_Logic structs to ensure zero-cost flat paths.
 */
enum class QueryOp : uint8_t {
    CULL, // Immediate bitwise pruning
    ADD   // Deferred appending via Wave/Graph Command Buffers
};

/**
 * QueryLogicValidator
 * The DOD Compile-Time Gatekeeper. Prevents invalid matrix pairings.
 */
struct QueryLogicValidator {
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
        if ((logic_layouts & view_layouts) == BufferLayoutType::NONE) return false;

        // 3. Payload DataType Intersection
        if ((logic_types & view_types) == DataType::NONE) return false;

        return true;
    }
};

/**
 * IsQueryLogic Concept (C++20)
 * Enforces the strict contract for bitwise Selection manipulators.
 */
template <typename T>
concept IsQueryLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::required_capabilities } -> std::convertible_to<ViewCapability>;
    { T::required_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::required_types } -> std::convertible_to<DataType>; // Registry pruning dependency
    { T::supports_cull } -> std::convertible_to<bool>;
    { T::supports_addition } -> std::convertible_to<bool>;
    { T::transient_workspace_bytes } -> std::convertible_to<size_t>;
    
    // Must implement pure 'execute' handling the selection mutation.
    { 
        std::declval<T>().template execute<QueryOp::CULL, typename T::DefaultView, typename T::DefaultStrategy>(
            std::declval<MemoryBufferSelectionPOD&>(), 
            std::declval<const TaskContextPOD&>(), 
            std::declval<typename T::DefaultView&>()
        ) 
    } -> std::same_as<void>;
};

} // namespace ideam::core