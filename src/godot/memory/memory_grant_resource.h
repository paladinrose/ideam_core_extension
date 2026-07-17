#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include "grant_part_resource.h"

#include "../../core/memory/memory_common.h"
#include "../../core/memory/memory_grant_pod.h"

namespace ideam::godot_ext {

class MemoryGrantResource : public godot::Resource {
    GDCLASS(MemoryGrantResource, godot::Resource)

public:
    enum GrantCapacity {
        CAPACITY_LITE = 4,   
        CAPACITY_HEAVY = 8   
    };

private:
    godot::StringName grant_name;
    GrantCapacity capacity_mode = CAPACITY_LITE;
    godot::TypedArray<GrantPartResource> configured_parts;

    // Emulation tracking parameters
    bool emulated_valid = true;
    godot::PackedStringArray emulation_errors;

protected:
    static void _bind_methods();

public:
    MemoryGrantResource() = default;
    ~MemoryGrantResource() = default;

    void set_grant_name(const godot::StringName& p_name);
    godot::StringName get_grant_name() const;

    void set_capacity_mode(int p_mode);
    int get_capacity_mode() const;

    void set_configured_parts(const godot::TypedArray<GrantPartResource>& p_parts);
    godot::TypedArray<GrantPartResource> get_configured_parts() const;

    godot::PackedInt32Array get_buffer_ids() const;
    
    bool add_part(const godot::Ref<GrantPartResource>& p_part);
    void remove_part(int p_index);
    void clear_parts();

    // Emulation state APIs called by MemoryManagerResource linter pass
    void clear_emulation_state();
    void add_emulation_error(const godot::String& p_error);
    
    bool is_emulated_valid() const { return emulated_valid; }
    godot::PackedStringArray get_emulation_errors() const { return emulation_errors; }

    bool validate_for_compilation() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::MemoryGrantResource::GrantCapacity);