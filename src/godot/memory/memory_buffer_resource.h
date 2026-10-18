#pragma once

#include "../../core/memory/memory_buffer_pod.h"
#include "../../core/memory/memory_common.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class MemoryBufferResource : public godot::Resource {
    GDCLASS(MemoryBufferResource, godot::Resource)

    using BufferLayoutType = ideam::core::BufferLayoutType;
    using BufferAlignmentMode = ideam::core::BufferAlignmentMode;
    using DataType = ideam::core::DataType;

public:
    enum SelectionMode {
        SELECTION_DENSE  = 0,
        SELECTION_SPARSE = 1
    };

private:
    godot::StringName buffer_name; 
    BufferLayoutType layout_type = BufferLayoutType::SOA; 
    BufferAlignmentMode alignment_mode = BufferAlignmentMode::STD430;
    int max_elements = 1024;
    int alignment = 64; // Base address alignment in Master Block
    bool needs_gpu_compute = false; 
    
    // Layout-Specific Parameters
    uint32_t elements_per_tile = 64;       // For TILED_SOA
    uint32_t page_size_bytes = 4096;       // For PAGED
    int64_t sparse_universe_size = 10000;  // For SPARSE_SET entity ID upper bound

    // Shadow Buffer Config
    bool enable_shadowing = false;  
    SelectionMode selection_mode = SELECTION_DENSE;
    
    godot::TypedArray<godot::Dictionary> columns;

protected:
    static void _bind_methods();

public:
    MemoryBufferResource() = default;
    ~MemoryBufferResource() = default;

    void set_buffer_name(const godot::StringName& p_name);
    godot::StringName get_buffer_name() const;

    void set_layout_type(int p_type);
    int get_layout_type() const;

    void set_max_elements(int p_elements);
    int get_max_elements() const;

    // Layout Specific Setters/Getters
    void set_elements_per_tile(int p_count);
    int get_elements_per_tile() const;

    void set_page_size_bytes(int p_bytes);
    int get_page_size_bytes() const;

    void set_sparse_universe_size(int64_t p_size);
    int64_t get_sparse_universe_size() const;

    // Computed Scaffolding Helpers (For View Layer)
    int get_element_stride() const;
    int get_tile_count() const;
    int get_page_count() const;

    // Alignment & Mode
    void set_alignment_mode(int p_mode);
    int get_alignment_mode() const;

    void set_alignment(int p_align);
    int get_alignment() const;

    void set_needs_gpu_compute(bool p_needs_gpu);
    bool get_needs_gpu_compute() const;

    void set_enable_shadowing(bool p_shadow);
    bool get_enable_shadowing() const;

    void set_selection_mode(int p_mode);
    int get_selection_mode() const;

    void set_columns(const godot::TypedArray<godot::Dictionary>& p_columns);
    godot::TypedArray<godot::Dictionary> get_columns() const;

    // Schema Management & Column Modification Helpers
    void add_column(const godot::StringName& p_name, int64_t p_data_type, int p_custom_size = 0);
    void remove_column(int p_index);
    void clear_columns();

    // Sizing & Memory Footprint Breakdown
    int calculate_raw_data_bytes() const;
    int calculate_unpadded_data_bytes() const;
    int calculate_padding_bytes() const;
    int calculate_shadow_bytes() const;
    int calculate_projected_footprint_bytes() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::MemoryBufferResource::SelectionMode);