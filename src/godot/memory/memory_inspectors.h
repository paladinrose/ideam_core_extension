#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../../core/memory/memory_buffer_selection_pod.h"
#include "../../core/memory/memory_grant_pod.h"
#include "../../core/memory/memory_manager_dod.h"

namespace ideam::godot_ext {

/**
 * MemorySelectionInspector
 * A lightweight, read-only snapshot of a MemoryBufferSelectionPOD.
 */
class MemorySelectionInspector : public godot::RefCounted {
    GDCLASS(MemorySelectionInspector, godot::RefCounted)

private:
    // Snapshot primitive data. We do NOT store pointers.
    int64_t element_count = 0;
    uint32_t target_buffer_id = 0;
    uint64_t buffer_version = 0;
    godot::String selection_mode_name;
    bool valid = false;

protected:
    static void _bind_methods();

public:
    MemorySelectionInspector() = default;
    ~MemorySelectionInspector() = default;

    // C++ Initialization Constructor
    void initialize_from_pod(const core::MemoryBufferSelectionPOD& p_pod);

    // Read-only GDScript API
    int get_element_count() const { return static_cast<int>(element_count); }
    int get_target_buffer_id() const { return static_cast<int>(target_buffer_id); }
    int get_buffer_version() const { return static_cast<int>(buffer_version); }
    godot::String get_selection_mode_string() const { return selection_mode_name; }
    bool is_valid() const { return valid; }
};

/**
 * MemoryGrantInspector
 * A lightweight, thread-safe, read-only snapshot of an active MemoryGrantPOD.
 * Provides the Editor UI with non-blocking access to telemetry and execution states.
 */
class MemoryGrantInspector : public godot::RefCounted {
    GDCLASS(MemoryGrantInspector, godot::RefCounted)

private:
    uint64_t manager_version = 0;
    bool active = false;
    bool emulated = false;

    // Telemetry and validation states
    bool error_state = false;
    bool dirty_state = false;
    
    // An array of dictionaries, each describing a GrantPartPOD
    godot::TypedArray<godot::Dictionary> part_snapshots;

protected:
    static void _bind_methods();

public:
    MemoryGrantInspector() = default;
    ~MemoryGrantInspector() = default;

    // C++ Initialization Templates
    template <core::IsMemoryGrant TGrant>
    void initialize_from_grant(const TGrant& p_grant) {
        manager_version = p_grant.manager_version_at_issue;
        active = p_grant.active;
        
        // Validation check against dangling pointers and layout mismatch
        error_state = !p_grant.is_valid();
        
        // Dirty flag for execution staleness. 
        // Can be hooked up directly to a dirty_parts_mask in future iterations.
        dirty_state = false; 
        
        part_snapshots.clear();

        for (uint32_t i = 0; i < p_grant.part_count; ++i) {
            const core::GrantPartPOD& part = p_grant.parts[i];
            
            godot::Dictionary dict;
            dict["buffer_id"] = part.buffer_id;
            dict["access_mode"] = part.access_mode == core::BufferAccessMode::WRITE ? "WRITE" : "READ";
            dict["element_stride"] = part.element_stride;
            dict["element_count"] = part.selection.element_count;
            dict["capacity"] = part.selection.capacity; // Required for node canvas telemetry
            
            part_snapshots.push_back(dict);
        }
    }

    /**
     * @brief Injects an artificial layout state derived from setup-time data (e.g., MemoryManagerResource).
     * Bypasses the need for an active runtime DOD structure to render node ports.
     */
    void setup_emulated_grant(const godot::TypedArray<godot::Dictionary>& p_mock_parts);
    
    // Read-only GDScript API
    int get_manager_version() const { return static_cast<int>(manager_version); }
    bool is_active() const { return active; }
    bool is_emulated() const { return emulated; }
    int get_part_count() const { return part_snapshots.size(); }
    
    godot::Dictionary get_part_snapshot(int p_index) const;
    bool has_error() const { return error_state; }
    bool is_dirty() const { return dirty_state; }

    godot::PackedInt32Array get_buffer_ids() const;
};

} // namespace ideam::godot_ext