#ifndef IDEAM_CORE_METADATA_TASK_H
#define IDEAM_CORE_METADATA_TASK_H

#include "metadata_logic/metadata_logic_traits.h"
#include "i_native_task.h"
#include "../memory/views/view_traits.h"

namespace ideam::core {

template<
    IsMetadataLogic T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
class alignas(64) MetadataTask final : public INativeTask {
    
    static_assert(
        MetadataLogicValidator::validate(
            T_Logic::requirements, 
            T_Logic::supported_layouts, 
            ViewTraits<T_View>::capabilities, 
            BufferLayoutType::NONE 
        ),
        "MetadataTask instantiation failed: Selected T_View does not fulfill T_Logic requirements!"
    );

private:
    T_Logic logic;

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    explicit MetadataTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~MetadataTask() override = default;

    // MetadataTasks do not cull selections proactively.
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {}

    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_id = logic.get_target_buffer_id();
        
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return; 
        
        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_id);
        if (!selection) return;

        T_View view = _create_view(p_context, part);
        
        logic.template execute_metadata<T_View, T_Strategy>(*selection, p_context, view);
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
        
        view.head_ptr = reinterpret_cast<VType*>(p_part->raw_base_ptr);
        view.count    = p_part->selection.capacity;
        
        if constexpr (requires { logic.configure_view(view, p_context, p_part); }) {
            logic.configure_view(view, p_context, p_part);
        }
        
        return view;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_METADATA_TASK_H