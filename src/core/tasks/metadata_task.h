#pragma once

#include "metadata_logic/metadata_logic_traits.h"
#include "i_native_task.h"
#include "../memory/views/view_traits.h"

#include <type_traits>
#include <new> // Required for hardware_destructive_interference_size

namespace ideam::core {

template<
    IsMetadataLogic T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
// Utilize hardware-aware cache line sizing to strictly prevent false sharing 
// when the Task Graph dispatches these blocks to parallel thread pools.
#ifdef __cpp_lib_hardware_interference_size
class alignas(std::hardware_destructive_interference_size) MetadataTask final : public INativeTask {
#else
class alignas(64) MetadataTask final : public INativeTask {
#endif
    
    // --- Compile-Time Firewall ---
    // Evaluates the strict bitwise intersection of what the Logic structural 
    // payload demands vs what the instantiated View can provide.
    static_assert(
        MetadataLogicValidator::validate<T_Logic, T_View>(),
        "MetadataTask instantiation failed: The selected T_View or T_Strategy does not fulfill the hardware, layout, or type requirements of the T_Logic!"
    );

private:
    T_Logic logic;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    MetadataTask() = default;

    // --- O(1) Pruning Bitmask ---
    // Expose the precise intersection of types. Guarantees the Factory Builder 
    // will only instantiate this pipeline if the runtime data aligns properly.
    static constexpr DataType supported_types = T_Logic::required_types & ViewTraits<T_View>::supported_types;

    explicit MetadataTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~MetadataTask() override = default;

    virtual void apply_properties(const godot::Dictionary& p_props) override { logic.apply_properties(p_props); }

    virtual void prepare(const TaskContextPOD& p_context) override {
        // Compile-time check: Only call prepare if the Logic struct defines it.
        // This keeps simple logic structs lightweight without forcing empty virtuals.
        if constexpr (requires { logic.prepare(p_context); }) {
            logic.prepare(p_context);
        }
    }

    virtual size_t get_transient_requirement(const TaskContextPOD& p_context) const override {
        // C++20 Compile-Time Route Resolution for Metadata Logic
        if constexpr (requires { logic.get_transient_requirement(p_context); }) {
            return logic.get_transient_requirement(p_context); 
        } else if constexpr (requires { T_Logic::transient_workspace_bytes; }) {
            return T_Logic::transient_workspace_bytes;         
        }
        return 0;
    }
    
    // MetadataTasks do not cull selections proactively.
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {}

    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return; 
        
        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);
        if (!selection) return;

        T_View view = assemble_view<T_Logic, T_View>(logic, p_context, part);
        
        logic.template execute<T_View, T_Strategy>(*selection, p_context, view);
    }

};

} // namespace ideam::core