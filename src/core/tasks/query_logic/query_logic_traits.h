#ifndef IDEAM_CORE_QUERY_LOGIC_TRAITS_H
#define IDEAM_CORE_QUERY_LOGIC_TRAITS_H

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
 * Bitmask defining the hardware or structural dependencies of a T_Logic.
 */
enum class LogicRequirement : uint32_t {
    NONE               = 0,
    REQUIRES_SPATIAL   = 1 << 0, 
    REQUIRES_SIMD      = 1 << 1, 
    REQUIRES_ATOMIC    = 1 << 2, 
    REQUIRES_PAGED     = 1 << 3, 
    READ_ONLY_DATA     = 1 << 4  
};

constexpr LogicRequirement operator|(LogicRequirement a, LogicRequirement b) noexcept {
    return static_cast<LogicRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has_logic_requirement(LogicRequirement p_mask, LogicRequirement p_req) noexcept {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_req)) != 0;
}

struct QueryLogicValidator {
    static constexpr bool validate(LogicRequirement requirements, BufferLayoutType supported_layouts, ViewCapability p_view_caps, BufferLayoutType p_buffer_layout) {
        if ((static_cast<uint32_t>(requirements) & static_cast<uint32_t>(LogicRequirement::REQUIRES_SPATIAL)) &&
            !has_capability(p_view_caps, ViewCapability::SPATIAL_ACCESS)) {
            return false;
        }

        if ((static_cast<uint32_t>(requirements) & static_cast<uint32_t>(LogicRequirement::REQUIRES_SIMD)) &&
            !has_capability(p_view_caps, ViewCapability::SIMD_ACCESS)) {
            return false;
        }

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
} && std::is_trivially_copyable_v<T>;

} // namespace ideam::core

#endif // IDEAM_CORE_QUERY_LOGIC_TRAITS_H