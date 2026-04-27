#pragma once

#include "i_native_task.h"

namespace ideam::core {

class alignas(64) EntryFillTask final : public INativeTask {
private:
    uint32_t target_buffer_id = 0;

public:
    // FIX 2: Default parameter added to satisfy parameterless construction in the NativeTaskRegistry
    explicit EntryFillTask(uint32_t p_buffer_id = 0) : target_buffer_id(p_buffer_id) {}

    // FIX 1: Implement the pure virtual prepare method. 
    // Since this task is 'final', the compiler can successfully devirtualize this 
    // and completely inline/strip the empty call during execution, avoiding a vtable jump.
    inline void prepare(const TaskContextPOD& p_context) override {}

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void execute(const TaskContextPOD& p_context) override {
        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_buffer_id);
        
        if (!selection) [[unlikely]] return; 

        // DOD Fast-Path: Highly predictable branch. If populated, pass through.
        if (selection->element_count > 0) [[likely]] {
            return;
        }

        // Trigger the Manager's SIMD inversion to populate the empty selection.
        if (p_context.manager) [[likely]] {
            p_context.manager->populate_inverse_selection(target_buffer_id, *selection);
        }
    }
    
    inline void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {}
};

} // namespace ideam::core