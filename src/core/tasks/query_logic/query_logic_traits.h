#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/view_traits.h"

#include "../i_native_task.h"

#include <concepts>
#include <type_traits>

#include <godot_cpp/variant/array.hpp>

namespace ideam::core {

enum class QueryOp : uint32_t {
    CULL = 0, 
    ADD,   
    Count
};

struct QueryLogicValidator {
    template <typename T_Logic, typename T_View>
    static consteval bool validate() {
        using VTraits = ViewTraits<T_View>;

        // 1. Hardware/Capability Access Validation
        if ((T_Logic::required_capabilities & VTraits::capabilities) != T_Logic::required_capabilities) return false;

        // 2. Structural Layout Intersection
        if ((T_Logic::required_layouts & VTraits::supported_layouts) == BufferLayoutType::NONE) return false;

        // 3. Payload DataType Intersection
        if ((T_Logic::required_types & VTraits::supported_types) == DataType::NONE) return false;

        // 4. Dimensionality Match
        if (T_Logic::dimensions != 0 && T_Logic::dimensions != VTraits::dimensions) return false;

        // 5. Stencil Kernel Contracts
        if (T_Logic::requires_static_kernel != VTraits::is_static_stencil) return false;
        if (T_Logic::requires_static_kernel && (T_Logic::kernel_size != VTraits::kernel_size)) return false;

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
    
    // Core Types & Caps
    { T::required_capabilities } -> std::convertible_to<ViewCapability>;
    { T::required_layouts } -> std::convertible_to<BufferLayoutType>;
    { T::required_types } -> std::convertible_to<DataType>; 

    // Explicit Spatial Contracts
    { T::dimensions } -> std::convertible_to<size_t>;
    { T::requires_static_kernel } -> std::convertible_to<bool>;
    { T::kernel_size } -> std::convertible_to<size_t>;

    // Sub-system configurations
    { T::supports_cull } -> std::convertible_to<bool>;
    { T::supports_addition } -> std::convertible_to<bool>;
    { T::transient_workspace_bytes } -> std::convertible_to<size_t>;
    { T::get_ui_properties() } -> std::same_as<godot::Array>;
    { std::declval<T>().apply_properties(std::declval<const godot::Dictionary&>()) } -> std::same_as<void>;
    
    // Must implement pure 'execute' handling the selection mutation.
    { 
        std::declval<T>().template execute<QueryOp::CULL, typename T::DefaultView, typename T::DefaultStrategy>(
            std::declval<MemoryBufferSelectionPOD&>(),
            std::declval<const TaskContextPOD&>(),
            std::declval<const typename T::DefaultView&>()
            //std::declval<const typename T::DefaultStrategy&>(),
        )
    } -> std::same_as<void>;
};

} // namespace ideam::core