#ifndef IDEAM_GODOT_MEMORY_MANAGER_RESOURCE_H
#define IDEAM_GODOT_MEMORY_MANAGER_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <memory>

#include "memory_buffer_resource.h"
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
    ScalabilityStrategy scaling_strategy = STRATEGY_FIXED;
    int transient_capacity_mb = 16; // Workspace memory for worker threads

    // The core backend. Instantiated during initialize_backend().
    std::shared_ptr<core::MemoryManagerDOD> backend_manager;

protected:
    static void _bind_methods();

public:
    MemoryManagerResource() = default;
    ~MemoryManagerResource() = default;

    // Core Orchestration
    void initialize_backend();
    bool is_initialized() const { return backend_manager != nullptr; }
    
    // Core Backend Access for Native Tasks
    std::shared_ptr<core::MemoryManagerDOD> get_backend() const { return backend_manager; }

    // Resource Properties
    void set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas) { buffer_schemas = p_schemas; }
    godot::TypedArray<MemoryBufferResource> get_buffer_schemas() const { return buffer_schemas; }

    void set_scaling_strategy(int p_strategy) { scaling_strategy = static_cast<ScalabilityStrategy>(p_strategy); }
    int get_scaling_strategy() const { return scaling_strategy; }

    void set_transient_capacity_mb(int p_mb) { transient_capacity_mb = p_mb; }
    int get_transient_capacity_mb() const { return transient_capacity_mb; }

    // UX Helpers
    godot::String get_projected_footprint_string() const;

    // FFI Passed Queries
    bool buffer_contains_id(int p_buffer_id, int p_entity_id) const;
    int get_dense_index(int p_buffer_id, int p_entity_id) const;
    void flush_gpu_updates();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::MemoryManagerResource::ScalabilityStrategy);

#endif // IDEAM_GODOT_MEMORY_MANAGER_RESOURCE_H