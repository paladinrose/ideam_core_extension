#ifndef IDEAM_CORE_I_NATIVE_TASK_H
#define IDEAM_CORE_I_NATIVE_TASK_H

#include "../memory/memory_grant_pod.h"
#include "../memory/memory_manager_dod.h"
#include "task_selection_command_pod.h"
#include "task_graph_command_pod.h"
#include <godot_cpp/variant/variant.hpp>

namespace ideam::core {

/**
 * TaskContextPOD
 * Passed to native tasks to provide raw access to their pre-baked memory.
 * Augments standard pointers with direct access to the Manager and GrantParts 
 * so custom T_Logic operations can instantiate lightweight DOD Views directly on the stack.
 */
struct TaskContextPOD {
    double delta;
    MemoryGrantPOD* grant;
    MemoryManagerDOD* manager; 

    // --- Command Buffers ---
    TaskGraphCommandPOD* graph_commands = nullptr;     // Tier 1: Processed after the entire Graph runs
    TaskSelectionCommandPOD* wave_commands = nullptr;  // Tier 2: Processed immediately after this Wave

    // --- View Instantiation Resources (DOD Fast Path) ---
    [[nodiscard]] const GrantPartPOD* get_grant_part(uint32_t p_buffer_id) const {
        if (!grant) return nullptr;
        for (uint32_t i = 0; i < grant->part_count; ++i) {
            if (grant->parts[i].buffer_id == p_buffer_id) {
                return &grant->parts[i];
            }
        }
        return nullptr;
    }

    // --- Legacy / Raw Contiguous Access ---
    [[nodiscard]] void* get_buffer_ptr(uint32_t p_buffer_id) const {
        if (!grant) return nullptr;
        for (uint32_t i = 0; i < grant->part_count; ++i) {
            if (grant->parts[i].buffer_id == p_buffer_id) {
                return grant->parts[i].raw_base_ptr;
            }
        }
        return nullptr;
    }

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
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) = 0;
    virtual void execute(const TaskContextPOD& p_context) = 0;
};

} // namespace ideam::core

#endif // IDEAM_CORE_I_NATIVE_TASK_H