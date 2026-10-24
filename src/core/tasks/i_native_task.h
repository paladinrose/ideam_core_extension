#pragma once

#include "../memory/memory_grant_pod.h"
#include "../memory/memory_manager_dod.h"
#include "task_selection_command_pod.h"
#include "task_graph_command_pod.h"
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/image.hpp>
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

    if (p_context.grant && p_part) {
        const uint32_t part_idx = static_cast<uint32_t>(p_part - p_context.grant->parts);
        const uint32_t mgr_ver = p_context.grant->global_manager_version_ptr
            ? static_cast<uint32_t>(*p_context.grant->global_manager_version_ptr)
            : 0;

        if constexpr (requires { view.grant; view.grant_part_index; }) {
            view.grant = p_context.grant;
            view.grant_part_index = part_idx;
        }
        if constexpr (requires { view.read_grant; view.read_part_idx; view.write_grant; view.write_part_idx; }) {
            view.read_grant = p_context.grant;
            view.read_part_idx = part_idx;
            view.write_grant = p_context.grant;
            view.write_part_idx = part_idx;
        }
        if constexpr (requires { view.parent_grant; view.parent_part_idx; view.child_grant; view.child_part_idx; }) {
            view.parent_grant = p_context.grant;
            view.parent_part_idx = part_idx;
            view.child_grant = p_context.grant;
            view.child_part_idx = part_idx;
        }
        if constexpr (requires { view.baked_buffer_version; }) {
            view.baked_buffer_version = p_part->buffer_version_at_issue;
        }
        if constexpr (requires { view.baked_manager_version; }) {
            view.baked_manager_version = mgr_ver;
        }
    }

    // 1. Primary Memory Binding (Agnostic to execution context)
    if constexpr (requires { view.bind(p_part); }) {
        view.bind(p_part);
    }

    // 2. Logic-Specific Configuration (Handles secondary buffers)
    if constexpr (requires { p_logic.configure_view(view, p_context, p_part); }) {
        p_logic.configure_view(view, p_context, p_part);
    }

    return view;
}

template <typename T_Logic>
static inline void apply_common_buffer_properties(T_Logic& r_logic, const godot::Dictionary& p_props) noexcept {
    uint32_t resolved_id = INVALID_ID;
    if (p_props.has("target_buffer_id")) {
        resolved_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
    } else if (p_props.has("grid_buffer_id")) {
        resolved_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_buffer_id"]));
    } else if (p_props.has("primary_buffer_id")) {
        resolved_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["primary_buffer_id"]));
    } else if (p_props.has("buffer_id")) {
        resolved_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
    }
    if (resolved_id != INVALID_ID) {
        if constexpr (requires { r_logic.target_buffer_id = resolved_id; }) {
            r_logic.target_buffer_id = resolved_id;
        }
        if constexpr (requires { r_logic.grid_buffer_id = resolved_id; }) {
            r_logic.grid_buffer_id = resolved_id;
        }
        if constexpr (requires { r_logic.primary_buffer_id = resolved_id; }) {
            r_logic.primary_buffer_id = resolved_id;
        }
        if constexpr (requires { r_logic.buffer_id = resolved_id; }) {
            r_logic.buffer_id = resolved_id;
        }
    }
}

class INativeTask {
public:
    virtual ~INativeTask() = default;
    
    virtual size_t get_transient_requirement(const TaskContextPOD& p_context) const { 
        return 0; 
    }

    virtual void apply_properties(const godot::Dictionary& p_props) {}
    virtual void prepare(const TaskContextPOD& p_context) = 0;
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) = 0;
    virtual void execute(const TaskContextPOD& p_context) = 0;
    
    virtual godot::Ref<godot::Image> get_image_snapshot() const { return godot::Ref<godot::Image>(); }
    virtual void request_snapshot() {}
    virtual void set_max_preview_size(int32_t p_size) {}
    [[nodiscard]] virtual int32_t get_max_preview_size() const { return 0; }
    [[nodiscard]] virtual bool is_snapshot_requested() const { return false; }

    
};


// Bit layout:
// [0-7]   Slot Index (0-255 slots max)
// [8]     Override Left Side? (1 bit)
// [9]     Enable Left? (1 bit)
// [10-25] Layout Type Left (16 bits)
// [26]    Override Right Side? (1 bit)
// [27]    Enable Right? (1 bit)
// [28-43] Layout Type Right (16 bits)
constexpr uint64_t pack_port_property_override(
    uint8_t slot, 
    bool override_left,  bool enable_left,  BufferLayoutType layout_left,
    bool override_right, bool enable_right, BufferLayoutType layout_right) 
{
    return (static_cast<uint64_t>(slot)) |
           (static_cast<uint64_t>(override_left) << 8)  | 
           (static_cast<uint64_t>(enable_left)   << 9)  |
           (static_cast<uint64_t>(layout_left)   << 10) |
           (static_cast<uint64_t>(override_right)<< 26) | 
           (static_cast<uint64_t>(enable_right)  << 27) |
           (static_cast<uint64_t>(layout_right)  << 28);
}

} // namespace ideam::core

 // IDEAM_CORE_I_NATIVE_TASK_H