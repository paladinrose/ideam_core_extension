#ifndef IDEAM_CORE_ENTRY_FILL_LOGIC_H
#define IDEAM_CORE_ENTRY_FILL_LOGIC_H

#include "../i_native_task.h"

namespace ideam::core {

/**
 * EntryFillLogic
 * A high-speed T_Logic struct designed for TaskGraphDOD's Fast Path.
 * Evaluates a MemoryBufferSelectionPOD:
 * - If it contains elements, it aborts (passing it along as-is for sub-graphs).
 * - If it is empty, it delegates to the MemoryManagerDOD to populate it
 * with an inversion of the buffer's global selection (filling it with available entries).
 */
struct EntryFillLogic {
    uint32_t target_buffer_id = 0;

    // Forces inline to avoid function call overhead in the task wave
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void execute(const TaskContextPOD& p_context) const {
        // Fetch the pre-allocated selection from the context's GrantPart
        MemoryBufferSelectionPOD* selection = p_context.get_selection(target_buffer_id);
        
        if (!selection) return; 

        // DOD Fast-Path: Abort if the selection is already populated (Sub-Graph pass-through)
        // This is a highly predictable branch for the CPU.
        if (selection->element_count > 0) {
            return;
        }

        // The selection is empty. Trigger the Manager's SIMD inversion to fill it.
        // This happens entirely in-place on pre-baked memory.
        if (p_context.manager) {
            p_context.manager->populate_inverse_selection(target_buffer_id, *selection);
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_ENTRY_FILL_LOGIC_H