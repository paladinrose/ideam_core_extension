#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <memory>

#include "memory_buffer_resource.h"
#include "managed_buffer_resource.h" 
#include "memory_grant_resource.h"
#include "memory_inspectors.h"

#include "../../core/memory/memory_manager_dod.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

class MemoryManagerResource : public godot::Resource {
    GDCLASS(MemoryManagerResource, godot::Resource)

public:
    enum ScalabilityStrategy {
        STRATEGY_FIXED,
        STRATEGY_SCALE_BY_RAM
    };

private:
    godot::Ref<IdeamUndoRedo> undo_redo;

    godot::TypedArray<MemoryBufferResource> buffer_schemas;
    godot::TypedArray<ManagedBufferResource> managed_buffer_schemas;
    godot::TypedArray<MemoryGrantResource> active_emulated_grants;

    ScalabilityStrategy scaling_strategy = STRATEGY_FIXED;
    int transient_capacity_mb = 16; 

    std::shared_ptr<core::MemoryManagerDOD> backend_manager;

    void _set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas);
    void _insert_buffer(int p_index, const godot::Ref<MemoryBufferResource>& p_buffer);
    void _remove_buffer(int p_index);
    void _move_buffer(int p_from_index, int p_to_index);

    void _set_scaling_strategy(int p_strategy);
    void _set_transient_capacity_mb(int p_mb);
    
    void _set_managed_buffers(const godot::TypedArray<ManagedBufferResource>& p_buffers);
    void _register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferResource>& p_buffers);
    void _clear_consumer_buffers(const godot::StringName& p_consumer);
    void _move_managed_buffer(int p_from_index, int p_to_index);

    void _set_active_emulated_grants(const godot::TypedArray<MemoryGrantResource>& p_grants);
    void _add_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant);
    void _insert_emulated_grant(int p_index, const godot::Ref<MemoryGrantResource>& p_grant);
    void _release_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant);
    void _clear_all_emulated_grants();
    
    
protected:
    static void _bind_methods();

public:
    MemoryManagerResource();
    ~MemoryManagerResource();

    void initialize_backend();
    bool is_initialized() const { return backend_manager != nullptr; }
    
#ifdef TOOLS_ENABLED
    void set_editor_undo_redo(godot::EditorUndoRedoManager* p_manager);
#endif
    godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo; }

    godot::TypedArray<godot::StringName> get_buffer_names() const;
    godot::TypedArray<godot::StringName> get_selected_buffer_names(const godot::PackedInt32Array& p_buffer_ids) const;
    
    std::shared_ptr<core::MemoryManagerDOD> get_backend() const;
    void set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas);
    godot::TypedArray<MemoryBufferResource> get_buffer_schemas() const;
    void move_buffer(int p_from_index, int p_to_index);
    void insert_buffer(int p_index, const godot::Ref<MemoryBufferResource>& p_buffer);
    void duplicate_buffer(int p_index);
    void remove_buffer(int p_index);
    
    void set_scaling_strategy(int p_strategy);
    int get_scaling_strategy() const;

    void set_transient_capacity_mb(int p_mb);
    int get_transient_capacity_mb() const;

    void clear_consumer_buffers(const godot::StringName& p_consumer);
    void register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferResource>& p_buffers);
    
    void set_managed_buffers(const godot::TypedArray<ManagedBufferResource>& p_buffers);
    godot::TypedArray<ManagedBufferResource> get_managed_buffers() const;
    void move_managed_buffer(int p_from_index, int p_to_index);

    void set_active_emulated_grants(const godot::TypedArray<MemoryGrantResource>& p_grants);
    godot::TypedArray<MemoryGrantResource> get_active_emulated_grants() const;

    int get_total_projected_footprint_bytes() const;

    godot::String get_projected_footprint_string() const;

    bool buffer_contains_id(int p_buffer_id, int p_entity_id) const;
    int get_dense_index(int p_buffer_id, int p_entity_id) const;

    godot::Ref<MemoryGrantResource> request_emulated_grant(const godot::PackedInt32Array& p_buffer_ids);
    void release_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant);
    void clear_all_emulated_grants();
    void recalculate_emulated_grants();

    godot::Ref<MemoryGrantInspector> get_grant_inspector(int p_grant_index) const;
    
    void serialize_subresources_to_disk();
};

} // namespace ideam::godot_ext