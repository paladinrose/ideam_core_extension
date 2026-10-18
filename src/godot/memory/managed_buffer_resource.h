#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/string.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_buffer_pod.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class MemoryManagerResource;

class ManagedBufferResource : public godot::Resource {
    GDCLASS(ManagedBufferResource, godot::Resource)

    using BufferLayoutType = ideam::core::BufferLayoutType;
    using BufferAlignmentMode = ideam::core::BufferAlignmentMode;
    using BufferLifecycleState = ideam::core::BufferLifecycleState;

private:
    godot::StringName consumer_name;
    godot::StringName purpose;

    // Dual-Mode References
    godot::Ref<MemoryBufferResource> schema_reference;
    int live_buffer_id = -1; // -1 represents INVALID_ID
    godot::Ref<MemoryManagerResource> manager_resource;

    // Fallback/AOT Profile Properties
    int fallback_layout_type = static_cast<int>(BufferLayoutType::FLAT);
    int fallback_byte_footprint = 0;
    int fallback_alignment = 64;

    // Synthetic Schema Cache for Raw Runtime Buffers
    mutable godot::Ref<MemoryBufferResource> synthetic_schema;
    mutable uint32_t cached_pod_version = 0;

    ideam::core::MemoryBufferPOD* _get_live_pod() const;

protected:
    static void _bind_methods();

public:
    ManagedBufferResource();
    ~ManagedBufferResource();

    // Identification & Purpose
    void set_consumer_name(const godot::StringName& p_name);
    godot::StringName get_consumer_name() const;

    void set_purpose(const godot::StringName& p_purpose);
    godot::StringName get_purpose() const;

    // Dual-Mode Bindings
    void set_schema_reference(const godot::Ref<MemoryBufferResource>& p_schema);
    godot::Ref<MemoryBufferResource> get_schema_reference() const;

    void set_live_buffer_id(int p_id);
    int get_live_buffer_id() const;

    void set_manager_resource(const godot::Ref<MemoryManagerResource>& p_manager);
    godot::Ref<MemoryManagerResource> get_manager_resource() const;

    // Fallback Properties (for unlinked profiles)
    void set_layout_type(int p_type);
    int get_layout_type() const;

    void set_byte_footprint(int p_bytes);
    int get_byte_footprint() const;

    void set_alignment(int p_alignment);
    int get_alignment() const;

    // Dual-Mode State Queries
    bool is_live() const;
    bool has_schema() const;
    int resolve_buffer_id() const;

    godot::StringName get_effective_buffer_name() const;
    int get_effective_layout_type() const;
    godot::Ref<MemoryBufferResource> get_effective_schema() const;

    // Ground-Truth Physical Telemetry
    int64_t get_element_count() const;
    int64_t get_max_elements() const;
    int64_t get_capacity_bytes() const;
    int64_t get_used_bytes() const;
    int64_t get_slack_bytes() const;
    double get_fragmentation_percentage() const;

    int get_lifecycle_state() const;
    godot::String get_lifecycle_state_string() const;

    int get_active_writer_count() const;
    int get_active_reader_count() const;
    bool is_vram_dirty() const;
};

} // namespace ideam::godot_ext