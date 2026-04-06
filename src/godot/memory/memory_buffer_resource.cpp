#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

void MemoryBufferResource::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_buffer_name", "name"), &MemoryBufferResource::set_buffer_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_buffer_name"), &MemoryBufferResource::get_buffer_name);
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_layout_type", "type"), &MemoryBufferResource::set_layout_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_layout_type"), &MemoryBufferResource::get_layout_type);
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_max_elements", "elements"), &MemoryBufferResource::set_max_elements);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_elements"), &MemoryBufferResource::get_max_elements);

    godot::ClassDB::bind_method(godot::D_METHOD("set_needs_gpu_compute", "needs_gpu"), &MemoryBufferResource::set_needs_gpu_compute);
    godot::ClassDB::bind_method(godot::D_METHOD("get_needs_gpu_compute"), &MemoryBufferResource::get_needs_gpu_compute);

    godot::ClassDB::bind_method(godot::D_METHOD("set_enable_shadowing", "enable"), &MemoryBufferResource::set_enable_shadowing);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enable_shadowing"), &MemoryBufferResource::get_enable_shadowing);

    godot::ClassDB::bind_method(godot::D_METHOD("set_columns", "columns"), &MemoryBufferResource::set_columns);
    godot::ClassDB::bind_method(godot::D_METHOD("get_columns"), &MemoryBufferResource::get_columns);

    godot::ClassDB::bind_method(godot::D_METHOD("calculate_projected_footprint_bytes"), &MemoryBufferResource::calculate_projected_footprint_bytes);

    // Updated godot::Variant::STRING to godot::Variant::STRING_NAME
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "buffer_name"), "set_buffer_name", "get_buffer_name");
    
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "layout_type", godot::PROPERTY_HINT_ENUM, "Flat:1,AOS:2,SOA:4,Sparse Set:8,Tiled SOA:16,Ring:32,Paged:64"), "set_layout_type", "get_layout_type");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_elements"), "set_max_elements", "get_max_elements");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "needs_gpu_compute"), "set_needs_gpu_compute", "get_needs_gpu_compute");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "enable_shadowing"), "set_enable_shadowing", "get_enable_shadowing");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "columns"), "set_columns", "get_columns");
}

int MemoryBufferResource::calculate_projected_footprint_bytes() const {
    int total_element_size = 0;
    
    for (int i = 0; i < columns.size(); ++i) {
        godot::Dictionary col = columns[i];
        if (col.has("size")) {
            total_element_size += static_cast<int>(col["size"]);
        }
    }
    
    int footprint = total_element_size * max_elements;

    if (enable_shadowing) {
        footprint += (16 * max_elements); 
    }

    return footprint;
}

} // namespace ideam::godot_ext