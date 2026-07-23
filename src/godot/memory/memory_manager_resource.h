#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <memory>

#include "memory_buffer_resource.h"
#include "managed_buffer_profile.h" 
#include "memory_grant_resource.h"
#include "../../core/memory/memory_manager_dod.h"

namespace ideam::godot_ext {

class MemoryManagerResource : public godot::Resource {
    GDCLASS(MemoryManagerResource, godot::Resource)

public:
    enum ScalabilityStrategy {
        STRATEGY_FIXED,
        STRATEGY_SCALE_BY_RAM
    };

private:
    godot::TypedArray<MemoryBufferResource> buffer_schemas;
    godot::TypedArray<ManagedBufferProfile> managed_profiles;
    godot::TypedArray<MemoryGrantResource> active_emulated_grants;

    ScalabilityStrategy scaling_strategy = STRATEGY_FIXED;
    int transient_capacity_mb = 16; 

    std::shared_ptr<core::MemoryManagerDOD> backend_manager;

protected:
    static void _bind_methods();

public:
    MemoryManagerResource() = default;
    ~MemoryManagerResource() = default;

    void initialize_backend();
    bool is_initialized() const { return backend_manager != nullptr; }
    
    godot::TypedArray<godot::StringName> get_buffer_names() const;
    godot::TypedArray<godot::StringName> get_selected_buffer_names(const godot::PackedInt32Array& p_buffer_ids) const;
    
    std::shared_ptr<core::MemoryManagerDOD> get_backend() const;
    void set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas);
    godot::TypedArray<MemoryBufferResource> get_buffer_schemas() const;
    
    void set_scaling_strategy(int p_strategy);
    int get_scaling_strategy() const;

    void set_transient_capacity_mb(int p_mb);
    int get_transient_capacity_mb() const;

    void clear_consumer_buffers(const godot::StringName& p_consumer);
    void register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferProfile>& p_profiles);
    
    void set_managed_profiles(const godot::TypedArray<ManagedBufferProfile>& p_profiles);
    godot::TypedArray<ManagedBufferProfile> get_managed_profiles() const;
    
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

    void serialize_subresources_to_disk();
};

} // namespace ideam::godot_ext

