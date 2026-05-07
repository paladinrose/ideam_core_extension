#pragma once

#include "i_native_task.h"
#include "query_logic/query_logic_traits.h"
#include "../memory/views/single_element_view.h"
#include "../memory/views/strategies.h"

#include <cstddef> 
#include <new> // Required for hardware_destructive_interference_size

namespace ideam::core {

template<
    IsQueryLogic T_Logic, 
    QueryOp Op          = QueryOp::CULL, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
// Utilize hardware-aware cache line sizing to strictly prevent false sharing 
// when the Task Graph dispatches these blocks to parallel thread pools.
#ifdef __cpp_lib_hardware_interference_size
class alignas(std::hardware_destructive_interference_size) QueryTask final : public INativeTask {
#else
class alignas(64) QueryTask final : public INativeTask {
#endif
    
    static_assert(
        (Op == QueryOp::CULL && T_Logic::supports_cull) ||
        (Op == QueryOp::ADD && T_Logic::supports_addition),
        "QueryTask instantiated with a QueryOp that the T_Logic does not support!"
    );

    // --- Compile-Time Firewall ---
    static_assert(
        QueryLogicValidator::validate(
            T_Logic::required_capabilities, 
            T_Logic::required_layouts,      
            T_Logic::required_types,        
            ViewTraits<T_View>::capabilities, 
            ViewTraits<T_View>::supported_layouts,
            ViewTraits<T_View>::supported_types
        ),
        "QueryTask instantiation failed: The selected T_View or T_Strategy does not fulfill the hardware, layout, or type requirements of the T_Logic!"
    );

private:
    T_Logic logic;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    QueryTask() = default;

    // --- O(1) Pruning Bitmask ---
    // Expose the precise intersection of types for the Factory Registry
    static constexpr DataType supported_types = T_Logic::required_types & ViewTraits<T_View>::supported_types;

    explicit QueryTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~QueryTask() override = default;

    virtual void apply_properties(const godot::Dictionary& p_props) override { logic.apply_properties(p_props); }

    virtual void prepare(const TaskContextPOD& p_context) override {
        if constexpr (requires { logic.prepare(p_context); }) {
            logic.prepare(p_context);
        }
    }

    virtual size_t get_transient_requirement(const TaskContextPOD& p_context) const override {
        if constexpr (requires { logic.get_transient_requirement(p_context); }) {
            return logic.get_transient_requirement(p_context); 
        } else if constexpr (requires { T_Logic::transient_workspace_bytes; }) {
            return T_Logic::transient_workspace_bytes;         
        }
        return 0;
    }
    
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        if constexpr (Op != QueryOp::CULL) return;

        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return; 

        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);
        if (!selection) return;

        T_View view = assemble_view<T_Logic, T_View>(logic, p_context, part);
        
        logic.template execute<Op, T_View, T_Strategy>(*selection, p_context, view);
    }

    virtual void execute(const TaskContextPOD& p_context) override {
        if constexpr (Op != QueryOp::ADD) return;

        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return;

        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);
        if (!selection) return;

        T_View view = assemble_view<T_Logic, T_View>(logic, p_context, part);
        
        logic.template execute<Op, T_View, T_Strategy>(*selection, p_context, view);
    }
};

} // namespace ideam::core