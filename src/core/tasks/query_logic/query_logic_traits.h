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
 * LogicRequirement
 * Bitmask defining the explicit hardware/structural dependencies of a T_Logic.
 */
enum class LogicRequirement : uint32_t {
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

constexpr LogicRequirement operator|(LogicRequirement a, LogicRequirement b) noexcept {
    return static_cast<LogicRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * QueryLogicValidator
 * The DOD Compile-Time Gatekeeper. Prevents invalid matrix pairings.
 */
struct QueryLogicValidator {
    static constexpr bool validate(LogicRequirement requirements, BufferLayoutType buffer_layout, ViewCapability p_view_caps, BufferLayoutType p_view_layout) {
        
        const uint32_t reqs = static_cast<uint32_t>(requirements);

        // If the Logic requires a capability, the View MUST provide it.
        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_SPATIAL)) &&
            !has_capability(p_view_caps, ViewCapability::SPATIAL_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_SIMD)) &&
            !has_capability(p_view_caps, ViewCapability::SIMD_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_ATOMIC)) &&
            !has_capability(p_view_caps, ViewCapability::ATOMIC_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_QUEUE)) &&
            !has_capability(p_view_caps, ViewCapability::QUEUE_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_STENCIL)) &&
            !has_capability(p_view_caps, ViewCapability::STENCIL_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_SWAP)) &&
            !has_capability(p_view_caps, ViewCapability::SWAP_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_ENTITY_ID)) &&
            !has_capability(p_view_caps, ViewCapability::ENTITY_ID_ACCESS)) return false;

        if ((reqs & static_cast<uint32_t>(LogicRequirement::REQUIRES_MULTI_COMPONENT)) &&
            !has_capability(p_view_caps, ViewCapability::MULTI_COMPONENT_ACCESS)) return false;

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
    
    { T::requirements } -> std::convertible_to<LogicRequirement>;
    { T::supported_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::supported_types } -> std::convertible_to<DataType>; // NEW: Registry pruning dependency
    { T::supports_cull } -> std::convertible_to<bool>;
    { T::supports_addition } -> std::convertible_to<bool>;

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

 // IDEAM_CORE_QUERY_LOGIC_TRAITS_H