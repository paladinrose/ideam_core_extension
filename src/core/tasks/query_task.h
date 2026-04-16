#ifndef IDEAM_CORE_QUERY_TASK_H
#define IDEAM_CORE_QUERY_TASK_H

#include "i_native_task.h"
#include "query_logic/query_logic_traits.h"
#include "../memory/views/single_element_view.h"
#include "../memory/views/strategies.h"
#include <cstddef> 

namespace ideam::core {

template<
    IsQueryLogic T_Logic, 
    QueryOp Op          = QueryOp::CULL, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
class alignas(64) QueryTask final : public INativeTask {
    
    static_assert(
        (Op == QueryOp::CULL && T_Logic::supports_cull) ||
        (Op == QueryOp::ADD && T_Logic::supports_addition),
        "QueryTask instantiated with a QueryOp that the T_Logic does not support!"
    );

    static_assert(
        QueryLogicValidator::validate(
            T_Logic::requirements, 
            T_Logic::supported_layouts, 
            ViewTraits<T_View>::capabilities, 
            BufferLayoutType::NONE
        ),
        "QueryTask instantiation failed: Selected T_View does not fulfill T_Logic requirements!"
    );

private:
    T_Logic logic;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    // --- NEW: Expose the supported types bitmask to the Registry Builder for O(1) pruning ---
    static constexpr DataType supported_types = T_Logic::supported_types;

    explicit QueryTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~QueryTask() override = default;

    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        // Only CULL operations manipulate the active working set bitmask immediately.
        if constexpr (Op != QueryOp::CULL) return;

        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return; // Silent abort if DAG failed to secure lease

        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);
        if (!selection) return;

        T_View view = _create_view(p_context, part);
        
        // Zero-overhead dispatch into the optimized logic payload
        logic.template execute<Op, T_View, T_Strategy>(*selection, p_context, view);
    }

    virtual void execute(const TaskContextPOD& p_context) override {
        // Only fire during the hot execution wave if we are defer-appending.
        if constexpr (Op != QueryOp::ADD) return;

        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return;

        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);
        if (!selection) return;

        T_View view = _create_view(p_context, part);
        
        logic.template execute<Op, T_View, T_Strategy>(*selection, p_context, view);
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T_View _create_view(const TaskContextPOD& p_context, const GrantPartPOD* p_part) const {
        T_View view;
        using VType = typename T_View::ValueType;
        
        view.head_ptr               = reinterpret_cast<VType*>(p_part->raw_base_ptr);
        view.count                  = p_part->selection.capacity;
        view.baked_buffer_version   = p_part->buffer_version_at_issue;
        view.baked_manager_version  = p_context.grant->manager_version_at_issue;
        
        return view;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_QUERY_TASK_H