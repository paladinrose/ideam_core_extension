#include "memory_buffer_resource.h"

namespace ideam::godot_ext {

void MemoryBufferResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_buffer_name", "name"), &MemoryBufferResource::set_buffer_name);
    ClassDB::bind_method(D_METHOD("get_buffer_name"), &MemoryBufferResource::get_buffer_name);
    
    ClassDB::bind_method(D_METHOD("set_layout_type", "type"), &MemoryBufferResource::set_layout_type);
    ClassDB::bind_method(D_METHOD("get_layout_type"), &MemoryBufferResource::get_layout_type);
    
    ClassDB::bind_method(D_METHOD("set_max_elements", "elements"), &MemoryBufferResource::set_max_elements);
    ClassDB::bind_method(D_METHOD("get_max_elements"), &MemoryBufferResource::get_max_elements);

    ClassDB::bind_method(D_METHOD("set_alignment", "alignment"), &MemoryBufferResource::set_alignment);
    ClassDB::bind_method(D_METHOD("get_alignment"), &MemoryBufferResource::get_alignment);

    ClassDB::bind_method(D_METHOD("set_needs_gpu_compute", "needs_gpu"), &MemoryBufferResource::set_needs_gpu_compute);
    ClassDB::bind_method(D_METHOD("get_needs_gpu_compute"), &MemoryBufferResource::get_needs_gpu_compute);

    ClassDB::bind_method(D_METHOD("set_enable_shadowing", "enable"), &MemoryBufferResource::set_enable_shadowing);
    ClassDB::bind_method(D_METHOD("get_enable_shadowing"), &MemoryBufferResource::get_enable_shadowing);

    ClassDB::bind_method(D_METHOD("set_selection_mode", "mode"), &MemoryBufferResource::set_selection_mode);
    ClassDB::bind_method(D_METHOD("get_selection_mode"), &MemoryBufferResource::get_selection_mode);

    ClassDB::bind_method(D_METHOD("set_columns", "columns"), &MemoryBufferResource::set_columns);
    ClassDB::bind_method(D_METHOD("get_columns"), &MemoryBufferResource::get_columns);

    ClassDB::bind_method(D_METHOD("calculate_projected_footprint_bytes"), &MemoryBufferResource::calculate_projected_footprint_bytes);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "buffer_name"), "set_buffer_name", "get_buffer_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_type", PROPERTY_HINT_ENUM, "Flat:1,AOS:2,SOA:4,Sparse Set:8,Tiled SOA:16,Ring:32,Paged:64"), "set_layout_type", "get_layout_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_elements"), "set_max_elements", "get_max_elements");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment"), "set_alignment", "get_alignment");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "needs_gpu_compute"), "set_needs_gpu_compute", "get_needs_gpu_compute");
    
    ADD_GROUP("Shadow Configuration", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_shadowing"), "set_enable_shadowing", "get_enable_shadowing");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "selection_mode", PROPERTY_HINT_ENUM, "Dense:0,Sparse:1"), "set_selection_mode", "get_selection_mode");
    
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "columns"), "set_columns", "get_columns");
}

int MemoryBufferResource::calculate_projected_footprint_bytes() const {
    int total_element_size = 0;
    
    for (int i = 0; i < columns.size(); ++i) {
        godot::Dictionary col = columns[i];
        if (col.has("size")) {
            total_element_size += static_cast<int>(col["size"]);
        }
    }
    
    // 1. Primary Data Footprint
    int footprint = total_element_size * max_elements;

    // 2. Shadow Buffer Footprint (Mirroring MemoryManagerDOD::create_shadowed_buffer math)
    if (enable_shadowing) {
        int selection_data_size = (selection_mode == SELECTION_DENSE) ? 
            (((max_elements + 63) / 64) * 8) : (max_elements * 8);

        int meta_soa_size = (max_elements * 4) + (max_elements * 8) + 
                            (max_elements * 4) + (max_elements * 1);

        footprint += (selection_data_size + meta_soa_size + 256);
    }

    return footprint;
}

} // namespace ideam::godot_ext