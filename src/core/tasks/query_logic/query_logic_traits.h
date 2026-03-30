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
 * LogicRequirement
 * Bitmask defining the hardware or structural dependencies of a T_Logic.
 */
enum class LogicRequirement : uint32_t {
    NONE               = 0,
    REQUIRES_SPATIAL   = 1 << 0, // Logic uses at(x,y,z) or neighbor stencils
    REQUIRES_SIMD      = 1 << 1, // Logic requires AOSOAView/get_lane()
    REQUIRES_ATOMIC    = 1 << 2, // Logic performs thread-safe aggregation
    REQUIRES_PAGED     = 1 << 3, // Logic optimized for virtualized memory
    READ_ONLY_DATA     = 1 << 4  // Hint for the scheduler: no write-back needed
};

constexpr LogicRequirement operator|(LogicRequirement a, LogicRequirement b) noexcept {
    return static_cast<LogicRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * QueryLogicTraits<T_Logic>
 * The definitive metadata provider for the Task Graph UI and Compiler.
 */
template<typename T_Logic>
struct QueryLogicTraits {
    // Basic structural requirements
    static constexpr LogicRequirement requirements = T_Logic::requirements;
    
    // Bitmask of BufferLayoutTypes this logic is physically capable of processing
    static constexpr BufferLayoutType supported_layouts = T_Logic::supported_layouts;

    // SIMD Width requirement (0 if scalar)
    static constexpr uint32_t required_lane_width = []() {
        if constexpr (requires { T_Logic::required_lane_width; }) {
            return T_Logic::required_lane_width;
        }
        return 0;
    }();

    /**
     * is_compatible
     * The primary filter for the Graph UI.
     * Returns true if the provided View and Buffer Layout satisfy all Logic constraints.
     */
    [[nodiscard]] static constexpr bool is_compatible(
        ViewCapability p_view_caps, 
        BufferLayoutType p_buffer_layout
    ) noexcept {
        // 1. Check Physical Layout Compatibility
        if (!has_layout(supported_layouts, p_buffer_layout)) return false;

        // 2. Check Spatial Constraint
        if ((static_cast<uint32_t>(requirements) & static_cast<uint32_t>(LogicRequirement::REQUIRES_SPATIAL)) &&
            !has_capability(p_view_caps, ViewCapability::SPATIAL_ACCESS)) {
            return false;
        }

        // 3. Check SIMD Constraint
        if ((static_cast<uint32_t>(requirements) & static_cast<uint32_t>(LogicRequirement::REQUIRES_SIMD)) &&
            !has_capability(p_view_caps, ViewCapability::SIMD_ACCESS)) {
            return false;
        }

        return true;
    }
};

/**
 * IsQueryLogic Concept (C++20)
 * Enforces the contract for any T_Logic used in a QueryTask.
 */
template <typename T>
concept IsQueryLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::requirements } -> std::convertible_to<LogicRequirement>;
    { T::supported_layouts } -> std::convertible_to<BufferLayoutType>;

    // Must implement execute_cull (The primary Query contract)
    { 
        std::declval<T>().template execute_cull<typename T::DefaultView, typename T::DefaultStrategy>(
            std::declval<MemoryBufferSelectionPOD&>(), 
            std::declval<const TaskContextPOD&>(), 
            std::declval<const typename T::DefaultView&>()
        )
    };
};

} // namespace ideam::core

#endif