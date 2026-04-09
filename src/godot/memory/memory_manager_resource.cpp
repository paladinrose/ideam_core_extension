#include "memory_manager_resource.h"
#include "../../core/memory/memory_common.h"
#include "../../core/memory/selection_utils.h" // Needed for core::SelectionMode if it's declared there
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>

namespace ideam::godot_ext {

void MemoryManagerResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("initialize_backend"), &MemoryManagerResource::initialize_backend);
    ClassDB::bind_method(D_METHOD("is_initialized"), &MemoryManagerResource::is_initialized);

    ClassDB::bind_method(D_METHOD("set_buffer_schemas", "schemas"), &MemoryManagerResource::set_buffer_schemas);
    ClassDB::bind_method(D_METHOD("get_buffer_schemas"), &MemoryManagerResource::get_buffer_schemas);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "buffer_schemas", PROPERTY_HINT_ARRAY_TYPE, "MemoryBufferResource"), "set_buffer_schemas", "get_buffer_schemas");

    ClassDB::bind_method(D_METHOD("set_scaling_strategy", "strategy"), &MemoryManagerResource::set_scaling_strategy);
    ClassDB::bind_method(D_METHOD("get_scaling_strategy"), &MemoryManagerResource::get_scaling_strategy);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scaling_strategy", PROPERTY_HINT_ENUM, "Fixed,Scale By RAM"), "set_scaling_strategy", "get_scaling_strategy");

    ClassDB::bind_method(D_METHOD("set_transient_capacity_mb", "mb"), &MemoryManagerResource::set_transient_capacity_mb);
    ClassDB::bind_method(D_METHOD("get_transient_capacity_mb"), &MemoryManagerResource::get_transient_capacity_mb);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transient_capacity_mb"), "set_transient_capacity_mb", "get_transient_capacity_mb");

    ClassDB::bind_method(D_METHOD("register_consumer_buffers", "consumer", "profiles"), &MemoryManagerResource::register_consumer_buffers);
    ClassDB::bind_method(D_METHOD("get_managed_profiles"), &MemoryManagerResource::get_managed_profiles);
    ClassDB::bind_method(D_METHOD("get_total_projected_footprint_bytes"), &MemoryManagerResource::get_total_projected_footprint_bytes);
    
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "managed_profiles", PROPERTY_HINT_ARRAY_TYPE, "ManagedBufferProfile"), "", "get_managed_profiles");
    
    ClassDB::bind_method(D_METHOD("get_projected_footprint_string"), &MemoryManagerResource::get_projected_footprint_string);
    ClassDB::bind_method(D_METHOD("buffer_contains_id", "buffer_id", "entity_id"), &MemoryManagerResource::buffer_contains_id);
    ClassDB::bind_method(D_METHOD("get_dense_index", "buffer_id", "entity_id"), &MemoryManagerResource::get_dense_index);
}

void MemoryManagerResource::register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferProfile>& p_profiles) {
    for (int i = managed_profiles.size() - 1; i >= 0; i--) {
        godot::Ref<ManagedBufferProfile> profile = managed_profiles[i];
        if (profile.is_valid() && profile->get_consumer_name() == p_consumer) {
            managed_profiles.remove_at(i);
        }
    }
    managed_profiles.append_array(p_profiles);
    emit_changed();
}

int MemoryManagerResource::get_total_projected_footprint_bytes() const {
    int total_bytes = 0;
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (schema.is_valid()) total_bytes += schema->calculate_projected_footprint_bytes();
    }
    for (int i = 0; i < managed_profiles.size(); ++i) {
        godot::Ref<ManagedBufferProfile> profile = managed_profiles[i];
        if (profile.is_valid()) total_bytes += profile->get_byte_footprint();
    }
    return total_bytes;
}

void MemoryManagerResource::initialize_backend() {
    if (backend_manager) return; 

    size_t total_capacity = static_cast<size_t>(get_total_projected_footprint_bytes());
    size_t transient_capacity = static_cast<size_t>(transient_capacity_mb) * 1024 * 1024;

    backend_manager = std::make_shared<core::MemoryManagerDOD>(total_capacity, transient_capacity);

    // 1. Process User Data Buffers
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (!schema.is_valid()) continue;

        core::BufferLayoutType layout = static_cast<core::BufferLayoutType>(schema->get_layout_type());
        uint32_t buffer_align = static_cast<uint32_t>(schema->get_alignment());
        int64_t max_elems = schema->get_max_elements();

        // Parse ColumnMetadata
        godot::TypedArray<godot::Dictionary> dict_columns = schema->get_columns();
        std::vector<core::ColumnMetadata> core_columns;
        size_t raw_data_size = 0;
        
        for (int j = 0; j < dict_columns.size(); ++j) {
            godot::Dictionary dict = dict_columns[j];
            core::ColumnMetadata col{}; // Zero-initialized 48-byte pod
            
            col.id = dict.has("id") ? static_cast<uint32_t>(dict["id"]) : j;
            col.type_size = dict.has("size") ? static_cast<uint32_t>(dict["size"]) : 1;
            col.alignment = dict.has("alignment") ? static_cast<uint32_t>(dict["alignment"]) : buffer_align;
            
            if (dict.has("data_type")) {
                col.data_type = static_cast<core::DataType>(static_cast<int64_t>(dict["data_type"]));
            } else {
                col.data_type = core::DataType::CUSTOM;
            }
            
            raw_data_size += (col.type_size * max_elems);
            core_columns.push_back(col);
        }

        uint32_t buffer_id = 0xFFFFFFFF;

        // Proper allocation dispatch based on MemoryManagerDOD actual signature
        if (schema->get_enable_shadowing()) {
            // MemoryManagerDOD expects its internal core::SelectionMode. Cast safely.
            core::SelectionMode sel_mode = static_cast<core::SelectionMode>(schema->get_selection_mode());
            buffer_id = backend_manager->create_shadowed_buffer(layout, raw_data_size, max_elems, sel_mode);
        } else {
            buffer_id = backend_manager->create_buffer(layout, raw_data_size, buffer_align);
        }

        if (buffer_id != 0xFFFFFFFF && !core_columns.empty()) {
            backend_manager->configure_buffer_columns(buffer_id, core_columns);
        }
    }

    // Note: Graph utility buffers are requested independently by the specific GraphDOD 
    // constructors using backend_manager->create_buffer(), but we guaranteed the space 
    // mathematically during get_total_projected_footprint_bytes().
}

godot::String MemoryManagerResource::get_projected_footprint_string() const {
    int total_bytes = get_total_projected_footprint_bytes();
    
    float mb = total_bytes / (1024.0f * 1024.0f);
    float trans_mb = static_cast<float>(transient_capacity_mb);
    float total_mb = mb + trans_mb;
    
    return godot::String("Projected Arena Size: ") + godot::String::num(total_mb, 2) + 
           " MB (" + godot::String::num(mb, 2) + " MB Data + " + 
           godot::String::num(trans_mb, 2) + " MB Transient)";
}

bool MemoryManagerResource::buffer_contains_id(int p_buffer_id, int p_entity_id) const {
    if (!backend_manager) return false;
    return backend_manager->buffer_contains_id(static_cast<uint32_t>(p_buffer_id), static_cast<uint32_t>(p_entity_id));
}

int MemoryManagerResource::get_dense_index(int p_buffer_id, int p_entity_id) const {
    if (!backend_manager) return -1;
    return backend_manager->get_dense_index(static_cast<uint32_t>(p_buffer_id), static_cast<uint32_t>(p_entity_id));
}

} // namespace ideam::godot_ext