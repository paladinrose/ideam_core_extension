#include "memory_manager_resource.h"
#include "../../core/memory/memory_common.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>

namespace ideam::godot_ext {

void MemoryManagerResource::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("initialize_backend"), &MemoryManagerResource::initialize_backend);
    godot::ClassDB::bind_method(godot::D_METHOD("is_initialized"), &MemoryManagerResource::is_initialized);

    godot::ClassDB::bind_method(godot::D_METHOD("set_buffer_schemas", "schemas"), &MemoryManagerResource::set_buffer_schemas);
    godot::ClassDB::bind_method(godot::D_METHOD("get_buffer_schemas"), &MemoryManagerResource::get_buffer_schemas);

    godot::ClassDB::bind_method(godot::D_METHOD("set_scaling_strategy", "strategy"), &MemoryManagerResource::set_scaling_strategy);
    godot::ClassDB::bind_method(godot::D_METHOD("get_scaling_strategy"), &MemoryManagerResource::get_scaling_strategy);

    godot::ClassDB::bind_method(godot::D_METHOD("set_transient_capacity_mb", "mb"), &MemoryManagerResource::set_transient_capacity_mb);
    godot::ClassDB::bind_method(godot::D_METHOD("get_transient_capacity_mb"), &MemoryManagerResource::get_transient_capacity_mb);

    godot::ClassDB::bind_method(godot::D_METHOD("get_projected_footprint_string"), &MemoryManagerResource::get_projected_footprint_string);

    // O(1) State Queries
    godot::ClassDB::bind_method(godot::D_METHOD("buffer_contains_id", "buffer_id", "entity_id"), &MemoryManagerResource::buffer_contains_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dense_index", "buffer_id", "entity_id"), &MemoryManagerResource::get_dense_index);
    godot::ClassDB::bind_method(godot::D_METHOD("flush_gpu_updates"), &MemoryManagerResource::flush_gpu_updates);

    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "scaling_strategy", godot::PROPERTY_HINT_ENUM, "Fixed,Scale By RAM"), "set_scaling_strategy", "get_scaling_strategy");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "transient_capacity_mb"), "set_transient_capacity_mb", "get_transient_capacity_mb");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "buffer_schemas", godot::PROPERTY_HINT_ARRAY_TYPE, "MemoryBufferResource"), "set_buffer_schemas", "get_buffer_schemas");
}

void MemoryManagerResource::initialize_backend() {
    int total_projected_bytes = 0;
    
    // 1. Calculate Master Block requirements
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (schema.is_valid()) {
            int schema_bytes = schema->calculate_projected_footprint_bytes();
            
            // Dynamic hardware tiering placeholder
            if (scaling_strategy == STRATEGY_SCALE_BY_RAM) {
                // Future expansion: Query OS physical memory limits and clamp max_elements 
                // on the schema before calculation to ensure we never hit system paging files.
            }
            
            total_projected_bytes += schema_bytes;
        }
    }

    size_t transient_bytes = static_cast<size_t>(transient_capacity_mb) * 1024 * 1024;
    
    // 2. Allocate the DOD Master Block (One giant contiguous allocation)
    backend_manager = std::make_shared<core::MemoryManagerDOD>(total_projected_bytes, transient_bytes);

    // 3. Topology Compilation: Slice the master block based on schemas
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (!schema.is_valid()) continue;
            
        core::BufferLayoutType target_layout = static_cast<core::BufferLayoutType>(schema->get_layout_type());
        int capacity = schema->calculate_projected_footprint_bytes(); 
        
        uint32_t buffer_id = 0;
        
        // C++ Layout Construction
        if (schema->get_enable_shadowing()) {
            // Hardcoding DENSE selection mode for the shadow buffer baseline. 
            buffer_id = backend_manager->create_shadowed_buffer(target_layout, capacity, schema->get_max_elements(), core::SelectionMode::DENSE);
        } else {
            // GPU resources mandate std140 or std430 (16-byte minimum) alignment. Cache alignment (64) otherwise.
            uint32_t base_alignment = schema->get_needs_gpu_compute() ? 16 : 64;
            buffer_id = backend_manager->create_buffer(target_layout, capacity, base_alignment);
        }

        // 4. Extract and pack ColumnMetadata
        godot::TypedArray<godot::Dictionary> gd_columns = schema->get_columns();
        std::vector<core::ColumnMetadata> core_columns;
        core_columns.reserve(gd_columns.size());

        for (int col_idx = 0; col_idx < gd_columns.size(); ++col_idx) {
            godot::Dictionary dict = gd_columns[col_idx];
            
            core::ColumnMetadata meta = {};
            
            // Extract from variant, provide safe fallbacks
            meta.id = dict.has("id") ? static_cast<uint32_t>(dict["id"]) : col_idx;
            meta.data_type = dict.has("type") ? static_cast<core::DataType>(static_cast<int>(dict["type"])) : core::DataType::FLOAT32;
            meta.type_size = dict.has("size") ? static_cast<uint32_t>(dict["size"]) : 4;
            
            // Default alignment to type size if not strictly declared by the user
            meta.alignment = dict.has("alignment") ? static_cast<uint32_t>(dict["alignment"]) : meta.type_size;
            
            // Initialize offsets to zero. The backend Manager will compute the exact 
            // byte offsets during configure_buffer_columns based on the layout topology.
            meta.offset = 0;
            meta.secondary_offset = 0;
            meta.tertiary_offset = 0;
            meta.current_size = 0;

            core_columns.push_back(meta);
        }

        // Finalize the structural blueprint for this slice
        if (!core_columns.empty()) {
            backend_manager->configure_buffer_columns(buffer_id, core_columns);
        }
    }
}

godot::String MemoryManagerResource::get_projected_footprint_string() const {
    int total_bytes = 0;
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (schema.is_valid()) {
            total_bytes += schema->calculate_projected_footprint_bytes();
        }
    }
    
    float mb = total_bytes / (1024.0f * 1024.0f);
    float trans_mb = static_cast<float>(transient_capacity_mb);
    
    return godot::String("Projected Impact: ") + godot::String::num(mb, 2) + " MB (Data) + " + godot::String::num(trans_mb, 2) + " MB (Transient Workspace)";
}

bool MemoryManagerResource::buffer_contains_id(int p_buffer_id, int p_entity_id) const {
    if (!backend_manager) return false;
    return backend_manager->buffer_contains_id(static_cast<uint32_t>(p_buffer_id), static_cast<uint32_t>(p_entity_id));
}

int MemoryManagerResource::get_dense_index(int p_buffer_id, int p_entity_id) const {
    if (!backend_manager) return -1;
    return backend_manager->get_dense_index(static_cast<uint32_t>(p_buffer_id), static_cast<uint32_t>(p_entity_id));
}

void MemoryManagerResource::flush_gpu_updates() {
    if (backend_manager) {
        backend_manager->flush_gpu_updates();
    }
}

} // namespace ideam::godot_ext