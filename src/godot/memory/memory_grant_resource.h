#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "grant_part_resource.h" // Depend directly on our sub-resource definition

// Core DOD headers for compile-time validation against the real execution structs
#include "../../core/memory/memory_common.h"
#include "../../core/memory/memory_grant_pod.h"

namespace ideam::godot_ext {

/**
 * @class MemoryGrantResource
 * @brief The Graph-level configuration surrogate for core::MemoryGrantPOD.
 * Enforces strict cache-line capacity limits (N=4 or N=8) during setup.
 * The graph compiler iterates over this Resource to allocate the hardware-aligned
 * MemoryGrantPOD needed for the actual simulation execution.
 */
class MemoryGrantResource : public godot::Resource {
    GDCLASS(MemoryGrantResource, godot::Resource)

public:
    enum GrantCapacity {
        CAPACITY_LITE = 4,   // Maps to MemoryGrantPOD (640 bytes / 10 Cache Lines)
        CAPACITY_HEAVY = 8   // Maps to MemoryGrantHeavyPOD (1152 bytes / 18 Cache Lines)
    };

private:
    GrantCapacity capacity_mode = CAPACITY_LITE;
    godot::TypedArray<GrantPartResource> configured_parts;

protected:
    static void _bind_methods();

public:
    MemoryGrantResource() = default;
    ~MemoryGrantResource() = default;

    void set_capacity_mode(int p_mode);
    int get_capacity_mode() const;

    void set_configured_parts(const godot::TypedArray<GrantPartResource>& p_parts);
    godot::TypedArray<GrantPartResource> get_configured_parts() const;

    // Setup-time helper to enforce hardware boundaries
    bool add_part(const godot::Ref<GrantPartResource>& p_part);
    void remove_part(int p_index);
    void clear_parts();

    /**
     * @brief Validates if the current UI configuration can be safely mapped 
     * to the underlying DOD cache-line structures.
     */
    bool validate_for_compilation() const;
};

} // namespace ideam::godot_ext

// Expose enum to Godot globally
VARIANT_ENUM_CAST(ideam::godot_ext::MemoryGrantResource::GrantCapacity);