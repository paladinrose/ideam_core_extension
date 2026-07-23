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

    // 1. Map Memory Buffer Resources[cite: 7, 8]
    TypedArray<MemoryBufferResource> schemas = memory_manager->get_buffer_schemas();
    for (int i = 0; i < schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = schemas[i];
        if (!schema.is_valid()) continue;

        Button* block = memnew(Button);
        block->set_text(schema->get_buffer_name());
        block->set_theme_type_variation("MemoryBufferBlock"); 
        
        // Let the blocks expand evenly for now. In the future, this is where you will 
        // calculate proportional stretch ratios based on the byte footprint of the buffer.
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        Array args;
        args.append(BLOCK_BUFFER);
        args.append(i);
        block->connect("pressed", Callable(this, "_on_block_pressed").bindv(args));
        
        add_child(block);
    }

    // 2. Map Managed Buffer Profiles[cite: 7, 8]
    TypedArray<ManagedBufferProfile> profiles = memory_manager->get_managed_profiles();
    for (int i = 0; i < profiles.size(); ++i) {
        Ref<ManagedBufferProfile> profile = profiles[i];
        if (!profile.is_valid()) continue;

        Button* block = memnew(Button);
        block->set_text(profile->get_consumer_name());
        block->set_theme_type_variation("ManagedProfileBlock");
        
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        Array args;
        args.append(BLOCK_PROFILE);
        args.append(i);
        block->connect("pressed", Callable(this, "_on_block_pressed").bindv(args));
        
        add_child(block);
    }

    // 3. Map Transient Capacity[cite: 7, 8]
    int transient_mb = memory_manager->get_transient_capacity_mb();
    if (transient_mb > 0) {
        Button* block = memnew(Button);
        block->set_text(String("Transient (") + String::num(transient_mb) + " MB)");
        block->set_theme_type_variation("TransientBlock");
        
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        Array args;
        args.append(BLOCK_TRANSIENT);
        args.append(0); // Transient doesn't use an array index, defaulting to 0
        block->connect("pressed", Callable(this, "_on_block_pressed").bindv(args));
        
        add_child(block);
    }
}

void MemoryRibbon::_on_block_pressed(int p_block_type, int p_index) {
    // Elevate the inspection request to the profiler window or inspector
    emit_signal("inspection_requested", p_block_type, p_index);
}

} // namespace ideam::godot_ext