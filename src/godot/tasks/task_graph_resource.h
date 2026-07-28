#pragma once

#include "../memory/memory_graph_resource.h"
#include "../../core/tasks/task_graph_dod.h"
#include "task_resource.h"
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <memory>

namespace ideam::godot_ext {

class TaskGraphResource : public MemoryGraphResource {
    GDCLASS(TaskGraphResource, MemoryGraphResource)

private:
    // --- Command Arena Capacities ---
    int command_arena_capacity_bytes = 1024 * 1024; // 1 MB default
    int selection_queue_capacity_elements = 250000;

    

protected:
    static void _bind_methods();
    
    godot::Ref<ManagedBufferResource> exec_buffer;
    godot::Ref<ManagedBufferResource> cmd_arena_buffer;
    godot::Ref<ManagedBufferResource> sel_arena_buffer;
    
    // Virtual pipeline overrides
    virtual void _ensure_managed_buffers() override;
    virtual void _gather_managed_buffers(godot::TypedArray<ManagedBufferResource>& r_managed_buffers) const override;

public:
    TaskGraphResource() = default;
    virtual ~TaskGraphResource() override = default;

    void set_command_arena_capacity_bytes(int p_bytes);
    int get_command_arena_capacity_bytes() const;

    void set_selection_queue_capacity_elements(int p_elements);
    int get_selection_queue_capacity_elements() const;

    // --- Explicit Profile Getters/Setters ---
    void set_exec_buffer(const godot::Ref<ManagedBufferResource>& p_buffer);
    godot::Ref<ManagedBufferResource> get_exec_buffer() const;

    void set_cmd_arena_buffer(const godot::Ref<ManagedBufferResource>& p_buffer);
    godot::Ref<ManagedBufferResource> get_cmd_arena_buffer() const;

    void set_sel_arena_buffer(const godot::Ref<ManagedBufferResource>& p_buffer);
    godot::Ref<ManagedBufferResource> get_sel_arena_buffer() const;

    std::shared_ptr<core::TaskGraphDOD> compile_to_task_graph(
        core::MemoryManagerDOD* p_manager, 
        godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext