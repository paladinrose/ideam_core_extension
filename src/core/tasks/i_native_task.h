#pragma once

#include "../memory/memory_grant_pod.h"
#include "../memory/memory_manager_dod.h"
#include "task_selection_command_pod.h"
#include "task_graph_command_pod.h"
#include <godot_cpp/variant/variant.hpp>
#include <cassert>

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

    // --- Transient Workspace ---
    void* local_workspace = nullptr;                   // Dedicated, lock-free transient memory for this Node

    // --- Command Queuing Methods (Inlined for DOD Fast Path) ---
    
    /**
     * queue_selection_command
     * Safely queues an index into the Tier 2 Wave Command Buffer.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool queue_selection_command(uint32_t p_target_buffer_id, int64_t p_index) const {
        if (!wave_commands) return false;
        
        if (wave_commands->count < wave_commands->capacity) {
            wave_commands->target_buffer_id = p_target_buffer_id;
            wave_commands->queued_indices[wave_commands->count++] = p_index;
            return true;
        }
        return false;
    }

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

template <typename T_Logic, typename T_View>
#if defined(_MSC_VER)
    [[msvc::forceinline]]
#else
    [[gnu::always_inline]]
#endif
static inline T_View assemble_view(const T_Logic& p_logic, const TaskContextPOD& p_context, const GrantPartPOD* p_part) {
    T_View view;
    
    // 1. Primary Memory Binding (Agnostic to execution context)
    if constexpr (requires { view.bind(p_part); }) {
        view.bind(p_part);
    }
    
    // 2. Logic-Specific Configuration (Handles secondary buffers)
    // Because p_logic knows about TaskContextPOD, IT does the heavy lifting.
    if constexpr (requires { p_logic.configure_view(view, p_context, p_part); }) {
        p_logic.configure_view(view, p_context, p_part);
    }
    
    return view;
}

class INativeTask {
public:
    virtual ~INativeTask() = default;
    
    virtual size_t get_transient_requirement(const TaskContextPOD& p_context) const { 
        return 0; 
    }
    virtual void prepare(const TaskContextPOD& p_context) = 0;
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) = 0;
    virtual void execute(const TaskContextPOD& p_context) = 0;
};

} // namespace ideam::core

 // IDEAM_CORE_I_NATIVE_TASK_H