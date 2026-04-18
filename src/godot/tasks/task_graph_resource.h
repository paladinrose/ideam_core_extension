#pragma once

#include "../memory/memory_graph_resource.h"
#include "../../core/tasks/task_graph_dod.h"
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

/**
 * TaskType
 * Godot-exposed mirror of core::TaskTypeDOD
 */
enum TaskType : uint32_t {
    TASK_GODOT_REFLECTION = 0,
    TASK_NATIVE_CPU       = 1,
    TASK_COMPUTE_GPU      = 2,
    TASK_QUERY_CULLER     = 3
};

class TaskGraphResource : public MemoryGraphResource {
    GDCLASS(TaskGraphResource, MemoryGraphResource)

private:
    // --- Command Arena Capacities ---
    // Defines the size of the raw byte arena backing the TaskGraphCommandPOD
    // Used for deferring Tier 1 structural extensions (spawning, resizing)
    int command_arena_capacity_bytes = 1024 * 1024; // 1 MB default

    // Defines the max number of int64_t indices the TaskSelectionCommandPODs can queue per frame
    // Used by tasks to dynamically expand query selections lock-free
    int selection_queue_capacity_elements = 250000;

protected:
    static void _bind_methods();
    
    // Virtual pipeline override to inject TaskGraph-specific utility footprints and Command Arenas
    virtual void _append_managed_profiles(godot::TypedArray<ManagedBufferProfile>& r_profiles) const override;

public:
    TaskGraphResource() = default;
    virtual ~TaskGraphResource() override = default;

    void set_command_arena_capacity_bytes(int p_bytes) { command_arena_capacity_bytes = p_bytes; emit_changed(); }
    int get_command_arena_capacity_bytes() const { return command_arena_capacity_bytes; }

    void set_selection_queue_capacity_elements(int p_elements) { selection_queue_capacity_elements = p_elements; emit_changed(); }
    int get_selection_queue_capacity_elements() const { return selection_queue_capacity_elements; }

    /**
     * @brief Compiles the Godot topological blueprint into an executable TaskGraphDOD.
     * Routes node configurations based on their serialized task_type and properties.
     */
    std::shared_ptr<core::TaskGraphDOD> compile_to_task_graph(
        core::MemoryManagerDOD* p_manager, 
        godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext

 // IDEAM_GODOT_TASK_GRAPH_RESOURCE_H