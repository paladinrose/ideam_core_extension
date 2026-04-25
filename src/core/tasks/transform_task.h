#pragma once

#include "transform_logic/transform_logic_traits.h"
#include "i_native_task.h"
#include "../memory/views/view_traits.h"

#include <new> // Required for hardware_destructive_interference_size

namespace ideam::core {

template<
    IsTransformLogic T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
// Utilize hardware-aware cache line sizing to strictly prevent false sharing 
// when the Task Graph dispatches these blocks to parallel thread pools.
#ifdef __cpp_lib_hardware_interference_size
class alignas(std::hardware_destructive_interference_size) TransformTask final : public INativeTask {
#else
class alignas(64) TransformTask final : public INativeTask {
#endif
    
    // --- Compile-Time Firewall ---
    // Evaluates the strict bitwise intersection of what the Logic mathematical 
    // payload demands vs what the instantiated View can structurally provide.
    static_assert(
        TransformLogicValidator::validate(
            T_Logic::requirements, 
            T_Logic::supported_layouts, 
            T_Logic::supported_types,
            ViewTraits<T_View>::capabilities,
            ViewTraits<T_View>::supported_layouts,
            ViewTraits<T_View>::supported_types
        ),
        "TransformTask instantiation failed: The selected T_View or T_Strategy does not fulfill the hardware, layout, or type requirements of the T_Logic!"
    );

private:
    T_Logic logic;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    TransformTask() = default;

    // --- O(1) Pruning Bitmask ---
    // Expose the precise intersection of types. This guarantees the Factory Builder 
    // will only instantiate this pipeline if the runtime Godot Variant type maps 
    // cleanly to both the View's storage format and the Logic's mathematical domain.
    static constexpr DataType supported_types = T_Logic::supported_types & ViewTraits<T_View>::supported_types;

    explicit TransformTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~TransformTask() override = default;

    virtual size_t get_transient_requirement(const TaskContextPOD& p_context) const override {
        // C++20 Compile-Time Route Resolution
        if constexpr (requires { logic.get_transient_requirement(p_context); }) {
            return logic.get_transient_requirement(p_context); // Dynamic Path
        } else if constexpr (requires { T_Logic::transient_workspace_bytes; }) {
            return T_Logic::transient_workspace_bytes;         // Static Fallback
        }
        return 0;
    }
    
    // Transforms are pure math/reductions, they do not prune bitmask selections.
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {}

    virtual void prepare(const TaskContextPOD& p_context) override {
        // Compile-time check: Only call prepare if the Logic struct defines it.
        // This keeps simple logic structs lightweight without forcing empty virtuals.
        if constexpr (requires { logic.prepare(p_context); }) {
            logic.prepare(p_context);
        }
    }

    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_id = logic.get_primary_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return; // Silent abort if DAG failed to secure lease
        
        T_View view = assemble_view<T_Logic, T_View>(logic, p_context, part);
        
        // Zero-overhead dispatch into the optimized math payload
        logic.template execute_transform<T_View, T_Strategy>(p_context, view);
    }

};

} // namespace ideam::core