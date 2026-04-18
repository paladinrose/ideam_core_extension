#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class MemoryBufferResource : public godot::Resource {
    GDCLASS(MemoryBufferResource, godot::Resource)

public:
    enum LayoutType {
        LAYOUT_FLAT       = 1 << 0,
        LAYOUT_AOS        = 1 << 1,
        LAYOUT_SOA        = 1 << 2,
        LAYOUT_SPARSE_SET = 1 << 3,
        LAYOUT_TILED_SOA  = 1 << 4,
        LAYOUT_RING       = 1 << 5,
        LAYOUT_PAGED      = 1 << 6
    };

    enum SelectionMode {
        SELECTION_DENSE  = 0,
        SELECTION_SPARSE = 1
    };

private:
    godot::StringName buffer_name; 
    LayoutType layout_type = LAYOUT_SOA; 
    int max_elements = 1024;
    int alignment = 64; // Added for cache/SIMD boundary tuning
    bool needs_gpu_compute = false; 
    
    // Shadow Buffer Config
    bool enable_shadowing = false;  
    SelectionMode selection_mode = SELECTION_DENSE;
    
    godot::TypedArray<godot::Dictionary> columns;

protected:
    static void _bind_methods();

public:
    MemoryBufferResource() = default;
    ~MemoryBufferResource() = default;

    void set_buffer_name(const godot::StringName& p_name) { buffer_name = p_name; }
    godot::StringName get_buffer_name() const { return buffer_name; }

    void set_layout_type(int p_type) { layout_type = static_cast<LayoutType>(p_type); }
    int get_layout_type() const { return layout_type; }

    void set_max_elements(int p_elements) { max_elements = p_elements; }
    int get_max_elements() const { return max_elements; }

    void set_alignment(int p_align) { alignment = p_align; }
    int get_alignment() const { return alignment; }

    void set_needs_gpu_compute(bool p_needs_gpu) { needs_gpu_compute = p_needs_gpu; }
    bool get_needs_gpu_compute() const { return needs_gpu_compute; }

    void set_enable_shadowing(bool p_shadow) { enable_shadowing = p_shadow; }
    bool get_enable_shadowing() const { return enable_shadowing; }

    void set_selection_mode(int p_mode) { selection_mode = static_cast<SelectionMode>(p_mode); }
    int get_selection_mode() const { return selection_mode; }

    void set_columns(const godot::TypedArray<godot::Dictionary>& p_columns) { columns = p_columns; }
    godot::TypedArray<godot::Dictionary> get_columns() const { return columns; }

    int calculate_projected_footprint_bytes() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::MemoryBufferResource::LayoutType);
VARIANT_ENUM_CAST(ideam::godot_ext::MemoryBufferResource::SelectionMode);

 // IDEAM_GODOT_MEMORY_BUFFER_RESOURCE_H