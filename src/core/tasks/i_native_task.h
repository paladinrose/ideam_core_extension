#ifndef IDEAM_CORE_I_NATIVE_TASK_H
#define IDEAM_CORE_I_NATIVE_TASK_H

#include "../memory/memory_grant_pod.h"
#include <godot_cpp/variant/variant.hpp>

namespace ideam::core {

/**
 * TaskContextPOD
 * Passed to native tasks to provide access to their pre-baked memory.
 */
struct TaskContextPOD {
    double delta;
    MemoryGrantPOD* grant;

    // Helper to get a specific buffer's raw pointer quickly
    [[nodiscard]] void* get_buffer_ptr(uint32_t p_buffer_id) const {
        if (!grant) return nullptr;
        for (uint32_t i = 0; i < grant->part_count; ++i) {
            if (grant->parts[i].buffer_id == p_buffer_id) {
                return grant->parts[i].raw_base_ptr;
            }
        }
        return nullptr;
    }

    // Helper to get the selection metadata for a specific part
    [[nodiscard]] MemoryBufferSelectionPOD* get_selection(uint32_t p_buffer_id) const {
        if (!grant) return nullptr;
        for (uint32_t i = 0; i < grant->part_count; ++i) {
            if (grant->parts[i].buffer_id == p_buffer_id) {
                return &grant->parts[i].selection;
            }
        }
        return nullptr;
    }
};

class INativeTask {
public:
    virtual ~INativeTask() = default;

    /**
     * cull_selections
     * Specialized tasks use this to narrow the MemoryBufferSelectionPOD.
     * p_dirty_mask: Bitmask indicating which GrantParts in the grant need re-querying.
     */
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) = 0;

    /**
     * execute
     * The hot-loop entry point. No Variants, no String names.
     * Should assume selections are already culled/valid.
     */
    virtual void execute(const TaskContextPOD& p_context) = 0;
    
    /**
     * get_task_name
     * For debugging/profiling.
     */
    virtual const char* get_task_name() const = 0;
};

} // namespace ideam::core

#endif // IDEAM_CORE_I_NATIVE_TASK_H