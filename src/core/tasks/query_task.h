#ifndef IDEAM_CORE_QUERY_TASK_H
#define IDEAM_CORE_QUERY_TASK_H

#include "i_native_task.h"
#include "../memory/views/single_element_view.h"
#include "../memory/views/strategies.h"
#include <cstddef> // Required for ptrdiff_t

namespace ideam::core {

template<
    typename T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
class QueryTask : public INativeTask {
    T_Logic logic;
    const char* task_name;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    QueryTask(const T_Logic& p_logic, const char* p_name = "QueryTask")
        : logic(p_logic), task_name(p_name) {}

    virtual ~QueryTask() override = default;

    [[nodiscard]] virtual const char* get_task_name() const override {
        return task_name;
    }

    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        // [DOD Update] Fetch by structural ID, not brittle array indices
        const uint32_t target_id = logic.get_target_buffer_id(); 
        
        // Safely extract the mutable selection and the const part info
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);

        if (!part || !selection) {
            return; // Buffer ID not present in this grant's cache line
        }

        // [DOD Update] Compute the spatial index via pointer arithmetic for the View.
        // This avoids maintaining dual state (index + ID) while satisfying view constraints.
        const uint32_t actual_idx = static_cast<uint32_t>(part - p_context.grant->parts);
        
        T_View view = _create_view(p_context, actual_idx, part);
        logic.template execute_cull<T_View, T_Strategy>(*selection, p_context, view);
    }

    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_id = logic.get_target_buffer_id();
        
        // Leverage the newly injected context helper rather than the raw grant
        const GrantPartPOD* part = p_context.get_grant_part(target_id);

        if (!part) return;

        const uint32_t actual_idx = static_cast<uint32_t>(part - p_context.grant->parts);
        
        T_View view = _create_view(p_context, actual_idx, part);
        logic.template execute_sim<T_View, T_Strategy>(p_context, view);
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T_View _create_view(const TaskContextPOD& p_context, uint32_t p_idx, const GrantPartPOD* p_part) const {
        T_View view;
        using VType = typename T_View::ValueType;
        
        view.head_ptr               = reinterpret_cast<VType*>(p_part->raw_base_ptr);
        view.grant                  = p_context.grant;
        view.grant_part_index       = p_idx;
        view.baked_buffer_version   = p_part->buffer_version_at_issue;
        view.baked_manager_version  = p_context.grant->manager_version_at_issue;

        return view;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_QUERY_TASK_H