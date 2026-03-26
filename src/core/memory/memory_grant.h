#ifndef IDEAM_CORE_MEMORY_GRANT_H
#define IDEAM_CORE_MEMORY_GRANT_H

#include "memory_common.h"
#include "memory_buffer_selection.h"
#include <godot_cpp/variant/rid.hpp> // Required for Godot RIDs
#include <vector>
#include <cstdint>

namespace ideam::core {

/**
 * GrantPart
 * Represents a secured claim to a specific MemoryBuffer.
 * This secures the ENTIRE buffer for the given selection set.
 */
struct GrantPart {
    // The ID of the buffer being secured.
    uint32_t buffer_id = 0xFFFFFFFF;
    
    // Safety check: Was the buffer moved/resized since this was issued?
    uint32_t buffer_version_at_issue = 0;

    // The "Where": The specific entities/cells secured by this part.
    const MemoryBufferSelection* selection = nullptr;
    uint64_t selection_version_at_issue = 0;

    // The "How": Are we reading or writing to this entire buffer selection?
    BufferAccessMode access_mode = BufferAccessMode::READ;
};

/**
 * MemoryGrant
 * A unified security token for SimulationGraph nodes.
 * Designed to be pooled and recycled by the MemoryManager.
 */
class MemoryGrant {
    friend class MemoryManager;

private:
    std::vector<GrantPart> parts;
    uint32_t manager_version_at_issue = 0;
    const uint32_t* global_manager_version_ptr = nullptr;
    bool active = false;

    // --- GPU Capabilities ---
    godot::RID uniform_set_rid;

public:
    MemoryGrant() = default;
    ~MemoryGrant() = default;

    /**
     * is_valid
     * Checks if the entire transaction (all parts) is still synchronized.
     */
#if defined(_MSC_VER)
    [[msvc::forceinline]]
#else
    [[gnu::always_inline]]
#endif
    inline bool is_valid() const {
        if (!active) return false;

        // Manager-level structural invalidation (e.g., Master Block rebase)
        if (global_manager_version_ptr && *global_manager_version_ptr != manager_version_at_issue) {
            return false;
        }

        // Buffer-level or Selection-level invalidation
        for (const auto& part : parts) {
            if (part.selection && part.selection->version != part.selection_version_at_issue) {
                return false;
            }
        }

        return true;
    }

    /**
     * has_buffer_access
     * Confirms if this grant provides authority for a specific buffer.
     */
    [[nodiscard]] inline bool has_buffer_access(uint32_t p_buffer_id, BufferAccessMode p_required_mode) const {
        for (const auto& part : parts) {
            if (part.buffer_id == p_buffer_id) {
                if (p_required_mode == BufferAccessMode::READ) {
                    return (part.access_mode != BufferAccessMode::NONE);
                }
                return (part.access_mode == p_required_mode || part.access_mode == BufferAccessMode::READ_WRITE);
            }
        }
        return false;
    }

    /**
     * get_uniform_set_rid
     * Returns the Godot RID for the Uniform Set bound to this grant's buffers.
     * Used by TaskGraph for GPU compute dispatch.
     */
    [[nodiscard]] inline godot::RID get_uniform_set_rid() const {
        return uniform_set_rid;
    }

    /**
     * get_parts
     * Provides access to the underlying buffer claims for Accessor initialization.
     */
    [[nodiscard]] const std::vector<GrantPart>& get_parts() const {
        return parts;
    }

    [[nodiscard]] bool is_active() const {
        return active;
    }

private:
    /**
     * set_parts
     * Used by MemoryManager to initialize a pooled grant with the current task's requirements.
     */
    void set_parts(const std::vector<GrantPart>& p_parts, 
                   uint32_t p_manager_version, 
                   const uint32_t* p_global_version_ptr,
                   godot::RID p_uniform_set = godot::RID()) {
        parts = p_parts;
        manager_version_at_issue = p_manager_version;
        global_manager_version_ptr = p_global_version_ptr;
        uniform_set_rid = p_uniform_set;
        active = true;
    }

    /**
     * _wipe
     * Internal reset for the MemoryManager's object pool.
     */
    void _wipe() {
        parts.clear();
        manager_version_at_issue = 0;
        global_manager_version_ptr = nullptr;
        uniform_set_rid = godot::RID();
        active = false;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_GRANT_H