#include "memory_ribbon.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryRibbon::_bind_methods() {
    ClassDB::bind_method(D_METHOD("sync_with_resource", "manager"), &MemoryRibbon::sync_with_resource);
    ClassDB::bind_method(D_METHOD("set_is_vertical", "vertical"), &MemoryRibbon::set_is_vertical);
    ClassDB::bind_method(D_METHOD("get_is_vertical"), &MemoryRibbon::get_is_vertical);
    
    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &MemoryRibbon::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_block_pressed", "block_type", "index"), &MemoryRibbon::_on_block_pressed);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_vertical"), "set_is_vertical", "get_is_vertical");
    
    // Custom signal to alert the profiler when a specific element on the ribbon is requested for inspection
    ADD_SIGNAL(MethodInfo("inspection_requested", PropertyInfo(Variant::INT, "block_type"), PropertyInfo(Variant::INT, "index")));
}

MemoryRibbon::MemoryRibbon() {
    // Default to a horizontal layout
    set_vertical(false);
    
    // Ensure the container fills its designated space in the UI
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

MemoryRibbon::~MemoryRibbon() {}

void MemoryRibbon::set_is_vertical(bool p_vertical) {
    set_vertical(p_vertical);
}

bool MemoryRibbon::get_is_vertical() const {
    return is_vertical();
}

void MemoryRibbon::sync_with_resource(Ref<MemoryManagerResource> p_manager) {
    if (memory_manager == p_manager) return;

    // Disconnect old resource if it exists
    if (memory_manager.is_valid() && memory_manager->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        memory_manager->disconnect("changed", Callable(this, "_on_resource_changed"));
    }

    memory_manager = p_manager;

    // Connect new resource to ensure the ribbon automatically redraws when data changes
    if (memory_manager.is_valid()) {
        memory_manager->connect("changed", Callable(this, "_on_resource_changed"));
    }

    _build_ribbon();
}

void MemoryRibbon::_on_resource_changed() {
    _build_ribbon();
}

void MemoryRibbon::_clear_ribbon() {
    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node* child = get_child(i);
        remove_child(child);
        child->queue_free();
    }
}

void MemoryRibbon::_build_ribbon() {
    _clear_ribbon();

    if (!memory_manager.is_valid()) {
        return;
    }

    // Determine the global capacity to calculate percentages[cite: 3, 4]
    float total_bytes = static_cast<float>(memory_manager->get_total_projected_footprint_bytes());
    if (total_bytes <= 0.0f) total_bytes = 1.0f; // Fallback to prevent division by zero

    // 1. Establish visual minimums for 0% buffers
    // A stretch ratio of 0.05 guarantees a minimum slice of space in the BoxContainer
    const float MIN_STRETCH_RATIO = 0.05f; 

    // 2. Map Memory Buffer Resources[cite: 1]
    TypedArray<MemoryBufferResource> schemas = memory_manager->get_buffer_schemas();
    for (int i = 0; i < schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = schemas[i];
        if (!schema.is_valid()) continue;

        Button* block = memnew(Button);
        block->set_text(schema->get_buffer_name());
        block->set_theme_type_variation("MemoryBufferBlock"); 
        
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        // Estimate footprint based on backend initialization logic
        int64_t max_elems = schema->get_max_elements();
        TypedArray<Dictionary> dict_columns = schema->get_columns();
        float raw_data_size = 0.0f;
        
        for (int j = 0; j < dict_columns.size(); ++j) {
            Dictionary dict = dict_columns[j];
            uint32_t type_size = dict.has("size") ? static_cast<uint32_t>(dict["size"]) : 1;
            raw_data_size += (type_size * max_elems);
        }
        
        // Double the estimate if shadowed, mimicking backend allocations[cite: 3]
        if (schema->get_enable_shadowing()) {
             raw_data_size *= 2.0f; 
        }

        // Apply proportional stretching
        float proportion = raw_data_size / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        Array args;
        args.append(BLOCK_BUFFER);
        args.append(i);
        block->connect("pressed", Callable(this, "_on_block_pressed").bindv(args));
        
        add_child(block);
    }

    // 3. Map Managed Buffer Profiles
    TypedArray<ManagedBufferResource> profiles = memory_manager->get_managed_buffers();
    for (int i = 0; i < profiles.size(); ++i) {
        Ref<ManagedBufferResource> profile = profiles[i];
        if (!profile.is_valid()) continue;

        Button* block = memnew(Button);
        block->set_text(profile->get_consumer_name());
        block->set_theme_type_variation("ManagedBufferBlock");
        
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        // Apply proportional stretching utilizing the profile's explicit byte footprint[cite: 3]
        float profile_bytes = static_cast<float>(profile->get_byte_footprint());
        float proportion = profile_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        Array args;
        args.append(BLOCK_MANAGED);
        args.append(i);
        block->connect("pressed", Callable(this, "_on_block_pressed").bindv(args));
        
        add_child(block);
    }

    // 4. Map Transient Capacity[cite: 1]
    int transient_mb = memory_manager->get_transient_capacity_mb();
    if (transient_mb > 0) {
        Button* block = memnew(Button);
        block->set_text(String("Transient (") + String::num(transient_mb) + " MB)");
        block->set_theme_type_variation("TransientBlock");
        
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        // Convert MB to Bytes and apply proportional stretching[cite: 3, 4]
        float transient_bytes = static_cast<float>(transient_mb) * 1024.0f * 1024.0f;
        float proportion = transient_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        Array args;
        args.append(BLOCK_TRANSIENT);
        args.append(0); 
        block->connect("pressed", Callable(this, "_on_block_pressed").bindv(args));
        
        add_child(block);
    }
}

void MemoryRibbon::_on_block_pressed(int p_block_type, int p_index) {
    // Elevate the inspection request to the profiler window or inspector
    emit_signal("inspection_requested", p_block_type, p_index);
}

} // namespace ideam::godot_ext