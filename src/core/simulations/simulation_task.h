#pragma once

#include "simulation_logic/simulation_logic_traits.h"
#include "../tasks/i_native_task.h"
#include "../memory/views/view_traits.h"

namespace ideam::core {

template<
    IsSimulationLogic T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
class alignas(64) SimulationTask final : public INativeTask {
    
    // --- Compile-Time Firewall ---
    static_assert(
        SimulationLogicValidator::validate(
            T_Logic::requirements, 
            T_Logic::supported_layouts, 
            ViewTraits<T_View>::capabilities, 
            BufferLayoutType::NONE // Layout validation happens dynamically or via graph bounds
        ),
        "SimulationTask instantiation failed: The selected T_View or T_Strategy does not fulfill the hardware/layout requirements of the T_Logic!"
    );

private:
    T_Logic logic;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    explicit SimulationTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~SimulationTask() override = default;

    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        if constexpr (requires { logic.cull_selections(p_context, p_dirty_mask); }) {
            logic.cull_selections(p_context, p_dirty_mask);
        }
    }

    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return; // Silent abort if DAG failed to secure lease
        
        // Instantiate the View entirely on the stack (zero allocation overhead)
        T_View view = assemble_view<T_Logic, T_View>(logic, p_context, part);
        
        // Zero-overhead dispatch into the optimized payload
        logic.template execute_sim<T_View, T_Strategy>(p_context, view);
    }

};

} // namespace ideam::core

 // IDEAM_CORE_SIMULATION_TASK_H