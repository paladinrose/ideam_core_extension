#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/view_traits.h"
#include "../../tasks/i_native_task.h"

#include <concepts>
#include <type_traits>

namespace ideam::core {

/**
 * SimulationRequirement
 * Bitmask defining the structural/hardware dependencies of a simulation logic struct.
 * Exposes requirements to both the compiler (for static_asserts) and the Godot UI.
 */
enum class SimulationRequirement : uint32_t {
    NONE                  = 0,
    REQUIRES_SPATIAL      = 1 << 0, // Requires neighbor lookups or 3D/4D folding
    REQUIRES_SIMD         = 1 << 1, // Demands AOSOA/vectorized lane access
    REQUIRES_ATOMIC       = 1 << 2, // Needs thread-safe aggregation (AtomicView)
    REQUIRES_SWAP         = 1 << 3, // Requires ping-pong buffering (State T -> T+1)
    MUTATES_TOPOLOGY      = 1 << 4  // Hint: Logic uses wave_commands to spawn/delete elements
};

constexpr SimulationRequirement operator|(SimulationRequirement a, SimulationRequirement b) noexcept {
    return static_cast<SimulationRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool has_sim_requirement(SimulationRequirement p_mask, SimulationRequirement p_req) noexcept {
    return (static_cast<uint32_t>(p_mask) & static_cast<uint32_t>(p_req)) != 0;
}

struct SimulationLogicValidator {
    static constexpr bool validate(SimulationRequirement requirements, BufferLayoutType supported_layouts, ViewCapability view_caps, BufferLayoutType buffer_layout) {
        
        // If the logic requires SIMD, the view MUST provide it.
        if (has_sim_requirement(requirements, SimulationRequirement::REQUIRES_SIMD) && 
            !has_capability(view_caps, ViewCapability::SIMD_ACCESS)) {
            return false;
        }

        // If logic requires spatial neighbor math, the view MUST be spatial.
        if (has_sim_requirement(requirements, SimulationRequirement::REQUIRES_SPATIAL) && 
            !has_capability(view_caps, ViewCapability::SPATIAL_ACCESS)) {
            return false;
        }

        return true;
    }
};

/**
 * IsSimulationLogic Concept (C++20)
 * Enforces the DOD contract for any T_Logic used in a SimulationTask.
 */
template <typename T>
concept IsSimulationLogic = requires {
    typename T::ValueType;
    typename T::DefaultView;
    typename T::DefaultStrategy;
    
    { T::requirements } -> std::convertible_to<SimulationRequirement>;
    { T::supported_layouts } -> std::convertible_to<BufferLayoutType>;
    
    // NEW: Forces the simulation to declare its transient footprint
    { T::transient_workspace_bytes } -> std::convertible_to<size_t>;

    // Must implement templated execute_sim handling the injected Context and View
    { 
        std::declval<T>().template execute_sim<typename T::DefaultView, typename T::DefaultStrategy>(
            std::declval<const TaskContextPOD&>(), 
            std::declval<typename T::DefaultView&>()
        ) 
    } -> std::same_as<void>;
} && std::is_trivially_copyable_v<T>;

} // namespace ideam::core

 // IDEAM_CORE_SIMULATION_LOGIC_TRAITS_H