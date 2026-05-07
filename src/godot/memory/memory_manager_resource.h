#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <memory>

#include "memory_buffer_resource.h"
#include "managed_buffer_profile.h" 
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
    
    std::shared_ptr<core::MemoryManagerDOD> get_backend() const { return backend_manager; }

    void set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas) { buffer_schemas = p_schemas; }
    godot::TypedArray<MemoryBufferResource> get_buffer_schemas() const { return buffer_schemas; }
    godot::TypedArray<godot::StringName> get_buffer_names() const;
    godot::TypedArray<godot::StringName> get_selected_buffer_names(const godot::PackedInt32Array& p_buffer_ids) const;
    
    void set_scaling_strategy(int p_strategy) { scaling_strategy = static_cast<ScalabilityStrategy>(p_strategy); }
    int get_scaling_strategy() const { return scaling_strategy; }

    void set_transient_capacity_mb(int p_mb) { transient_capacity_mb = p_mb; }
    int get_transient_capacity_mb() const { return transient_capacity_mb; }

    void register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferProfile>& p_profiles);
    godot::TypedArray<ManagedBufferProfile> get_managed_profiles() const { return managed_profiles; }
    int get_total_projected_footprint_bytes() const;

    godot::String get_projected_footprint_string() const;

    bool buffer_contains_id(int p_buffer_id, int p_entity_id) const;
    int get_dense_index(int p_buffer_id, int p_entity_id) const;
};

} // namespace ideam::godot_ext

