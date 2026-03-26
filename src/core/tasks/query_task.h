#ifndef IDEAM_CORE_QUERY_TASK_H
#define IDEAM_CORE_QUERY_TASK_H

#include "i_native_task.h"
#include "../memory/views/single_element_view.h"
#include "../memory/views/strategies.h"

namespace ideam::core {

/**
 * QueryTask<T_Logic, T_View, T_Strategy>
 * * The Task handles the "Administrative" work:
 * 1. Resolving the MemoryGrant parts.
 * 2. Ensuring version stability.
 * 3. Constructing the View and Strategy.
 * * The Logic handles the "Industrial" work:
 * 1. Filtering the selection bitmasks.
 * 2. Performing simulation math.
 */
template<
    typename T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
class QueryTask : public INativeTask {
    T_Logic logic;
    const char* task_name;

public:
    // Expose types for Graph UI introspection
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    QueryTask(const T_Logic& p_logic, const char* p_name = "QueryTask")
        : logic(p_logic), task_name(p_name) {}

    virtual ~QueryTask() override = default;

    [[nodiscard]] virtual const char* get_task_name() const override {
        return task_name;
    }

    /**
     * cull_selections
     */
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        const uint32_t target_idx = logic.get_target_part_index();
        if (target_idx >= p_context.grant->part_count) {
            return;
        }

        GrantPartPOD* part = const_cast<GrantPartPOD*>(&p_context.grant->parts[target_idx]);
        
        // Culling logic often needs a view of the data to decide what to prune
        T_View view = _create_view(p_context, target_idx, part);
        logic.template execute_cull<T_View, T_Strategy>(part->selection, p_context, view);
    }

    /**
     * execute
     */
    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_idx = logic.get_target_part_index();
        const GrantPartPOD* part = p_context.grant->get_part(target_idx);

        if (!part) return;

        T_View view = _create_view(p_context, target_idx, part);
        logic.template execute_sim<T_View, T_Strategy>(p_context, view);
    }

private:
    /**
     * _create_view
     * Encapsulates the boilerplate of mapping a GrantPart to a View.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T_View _create_view(const TaskContextPOD& p_context, uint32_t p_idx, const GrantPartPOD* p_part) const {
        T_View view;
        // Map raw pointers using the View's expected value type
        using VType = typename T_View::ValueType;
        
        view.head_ptr               = reinterpret_cast<VType*>(p_part->raw_base_ptr);
        view.grant                  = p_context.grant;
        view.grant_part_index       = p_idx;
        view.baked_buffer_version   = p_part->buffer_version_at_issue;
        view.baked_manager_version  = p_context.grant->manager_version_at_issue;

        // Note: Strategy state (if any) is assumed to be initialized by the view 
        // or defaulted here if the strategy has member data.
        return view;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_QUERY_TASK_H