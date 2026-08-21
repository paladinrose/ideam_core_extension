#include "memory_manager_resource.h"
#include "../../core/memory/memory_common.h"
#include "../../core/memory/selection_utils.h" // Needed for core::SelectionMode if it's declared there

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <map>
#include <vector>

namespace ideam::godot_ext {

void MemoryManagerResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("initialize_backend"), &MemoryManagerResource::initialize_backend);
    ClassDB::bind_method(D_METHOD("is_initialized"), &MemoryManagerResource::is_initialized);

    ClassDB::bind_method(D_METHOD("set_buffer_schemas", "schemas"), &MemoryManagerResource::set_buffer_schemas);
    ClassDB::bind_method(D_METHOD("get_buffer_schemas"), &MemoryManagerResource::get_buffer_schemas);
    ClassDB::bind_method(D_METHOD("get_buffer_names"), &MemoryManagerResource::get_buffer_names);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "buffer_schemas", PROPERTY_HINT_ARRAY_TYPE, "MemoryBufferResource"), "set_buffer_schemas", "get_buffer_schemas");

    ClassDB::bind_method(D_METHOD("move_buffer", "from_index", "to_index"), &MemoryManagerResource::move_buffer);
    ClassDB::bind_method(D_METHOD("insert_buffer", "index", "buffer"), &MemoryManagerResource::insert_buffer);
    ClassDB::bind_method(D_METHOD("duplicate_buffer", "index"), &MemoryManagerResource::duplicate_buffer);
    ClassDB::bind_method(D_METHOD("remove_buffer", "index"), &MemoryManagerResource::remove_buffer);
    ClassDB::bind_method(D_METHOD("move_buffers_bulk", "buffer_ids", "target_index"), &MemoryManagerResource::move_buffers_bulk);
    ClassDB::bind_method(D_METHOD("duplicate_buffers_bulk", "buffer_ids", "target_index"), &MemoryManagerResource::duplicate_buffers_bulk);

    ClassDB::bind_method(D_METHOD("set_scaling_strategy", "strategy"), &MemoryManagerResource::set_scaling_strategy);
    ClassDB::bind_method(D_METHOD("get_scaling_strategy"), &MemoryManagerResource::get_scaling_strategy);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scaling_strategy", PROPERTY_HINT_ENUM, "Fixed,Scale By RAM"), "set_scaling_strategy", "get_scaling_strategy");

    ClassDB::bind_method(D_METHOD("set_transient_capacity_mb", "mb"), &MemoryManagerResource::set_transient_capacity_mb);
    ClassDB::bind_method(D_METHOD("get_transient_capacity_mb"), &MemoryManagerResource::get_transient_capacity_mb);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transient_capacity_mb"), "set_transient_capacity_mb", "get_transient_capacity_mb");

    ClassDB::bind_method(D_METHOD("register_consumer_buffers", "consumer", "consumer_buffers"), &MemoryManagerResource::register_consumer_buffers);
    ClassDB::bind_method(D_METHOD("clear_consumer_buffers", "consumer"), &MemoryManagerResource::clear_consumer_buffers);
    ClassDB::bind_method(D_METHOD("set_managed_buffers", "managed_buffers"), &MemoryManagerResource::set_managed_buffers);
    ClassDB::bind_method(D_METHOD("get_managed_buffers"), &MemoryManagerResource::get_managed_buffers);
    ClassDB::bind_method(D_METHOD("move_managed_buffer", "from_index", "to_index"), &MemoryManagerResource::move_managed_buffer);
    
    ClassDB::bind_method(D_METHOD("get_total_projected_footprint_bytes"), &MemoryManagerResource::get_total_projected_footprint_bytes);
    
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "managed_buffer_schemas", PROPERTY_HINT_ARRAY_TYPE, "ManagedBufferProfile"), "set_managed_buffers", "get_managed_buffers");
    
    ClassDB::bind_method(D_METHOD("get_projected_footprint_string"), &MemoryManagerResource::get_projected_footprint_string);
    ClassDB::bind_method(D_METHOD("buffer_contains_id", "buffer_id", "entity_id"), &MemoryManagerResource::buffer_contains_id);
    ClassDB::bind_method(D_METHOD("get_dense_index", "buffer_id", "entity_id"), &MemoryManagerResource::get_dense_index);
    
    ClassDB::bind_method(D_METHOD("request_emulated_grant", "buffer_ids"), &MemoryManagerResource::request_emulated_grant);
    ClassDB::bind_method(D_METHOD("release_emulated_grant", "grant"), &MemoryManagerResource::release_emulated_grant);
    ClassDB::bind_method(D_METHOD("clear_all_emulated_grants"), &MemoryManagerResource::clear_all_emulated_grants);
    ClassDB::bind_method(D_METHOD("recalculate_emulated_grants"), &MemoryManagerResource::recalculate_emulated_grants);
    
    ClassDB::bind_method(D_METHOD("get_active_emulated_grants"), &MemoryManagerResource::get_active_emulated_grants);
    ClassDB::bind_method(D_METHOD("set_active_emulated_grants", "grants"), &MemoryManagerResource::set_active_emulated_grants);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "active_emulated_grants", PROPERTY_HINT_ARRAY_TYPE, "MemoryGrantResource"), "set_active_emulated_grants", "get_active_emulated_grants");
}

MemoryManagerResource::MemoryManagerResource() {}

MemoryManagerResource::~MemoryManagerResource() {}

godot::TypedArray<godot::StringName> MemoryManagerResource::get_buffer_names() const {
    godot::TypedArray<godot::StringName> names;
    
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (schema.is_valid()) {
            names.push_back(schema->get_buffer_name());
        }
    }
    
    return names;
}

godot::TypedArray<godot::StringName> MemoryManagerResource::get_selected_buffer_names(const godot::PackedInt32Array& p_buffer_ids) const {
    godot::TypedArray<godot::StringName> names;

    for (int i = 0; i < p_buffer_ids.size(); ++i) {
        uint32_t id = static_cast<uint32_t>(p_buffer_ids[i]);
        
        // Relies on 1:1 schema index assumption
        if (id < static_cast<uint32_t>(buffer_schemas.size())) {
            godot::Ref<MemoryBufferResource> schema = buffer_schemas[id];
            if (schema.is_valid()) {
                names.push_back(schema->get_buffer_name());
            }
        }
    }

    return names;
}

std::shared_ptr<core::MemoryManagerDOD> MemoryManagerResource::get_backend() const { return backend_manager; }

void MemoryManagerResource::set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas) {
    if (p_schemas == buffer_schemas) return;
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Set Buffer Schemas");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_buffer_schemas).bind(p_schemas.duplicate()));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_buffer_schemas).bind(buffer_schemas.duplicate()));
        undo_redo->commit_action();
    } else {
        _set_buffer_schemas(p_schemas);
    }
}
void MemoryManagerResource::_set_buffer_schemas(const godot::TypedArray<MemoryBufferResource>& p_schemas) { 
    buffer_schemas = p_schemas;
    emit_changed();
}
godot::TypedArray<MemoryBufferResource> MemoryManagerResource::get_buffer_schemas() const { return buffer_schemas; }

void MemoryManagerResource::move_buffer(int p_from_index, int p_to_index) {
    if (p_from_index < 0 || p_from_index >= buffer_schemas.size()) return;
    if (p_to_index < 0 || p_to_index >= buffer_schemas.size()) return;
    if (p_from_index == p_to_index) return;

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Move Memory Buffer");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_move_buffer).bind(p_from_index, p_to_index));
        // The inverse of a move is just moving it back from the destination to the origin
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_move_buffer).bind(p_to_index, p_from_index));
        undo_redo->commit_action();
    } else {
        _move_buffer(p_from_index, p_to_index);
    }
}

void MemoryManagerResource::_move_buffer(int p_from_index, int p_to_index) {
    godot::Variant buffer = buffer_schemas[p_from_index];
    buffer_schemas.remove_at(p_from_index);
    buffer_schemas.insert(p_to_index, buffer);
    emit_changed();
}

void MemoryManagerResource::remove_buffer(int p_index) {
    if (p_index < 0 || p_index >= buffer_schemas.size()) return;

    // Capture the existing reference so we can put it back during Undo
    godot::Ref<MemoryBufferResource> buffer_to_remove = buffer_schemas[p_index];

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Remove Memory Buffer");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_remove_buffer).bind(p_index));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_insert_buffer).bind(p_index, buffer_to_remove));
        undo_redo->commit_action();
    } else {
        _remove_buffer(p_index);
    }
}

void MemoryManagerResource::_remove_buffer(int p_index) {
    buffer_schemas.remove_at(p_index);
    emit_changed();
}

void MemoryManagerResource::insert_buffer(int p_index, const godot::Ref<MemoryBufferResource>& p_buffer) {
    if (!p_buffer.is_valid()) return;

    // Clamp the index to prevent out-of-bounds insertions if called directly from the UI
    int safe_index = p_index;
    if (safe_index < 0 || safe_index > buffer_schemas.size()) {
        safe_index = buffer_schemas.size();
    }

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Insert Memory Buffer");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_insert_buffer).bind(safe_index, p_buffer));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_remove_buffer).bind(safe_index));
        undo_redo->commit_action();
    } else {
        _insert_buffer(safe_index, p_buffer);
    }
}
void MemoryManagerResource::_insert_buffer(int p_index, const godot::Ref<MemoryBufferResource>& p_buffer) {
    // If the index is out of bounds, push to the end to prevent crashing
    if (p_index < 0 || p_index > buffer_schemas.size()) {
        buffer_schemas.push_back(p_buffer);
    } else {
        buffer_schemas.insert(p_index, p_buffer);
    }
    emit_changed();
}

void MemoryManagerResource::duplicate_buffer(int p_index) {
    if (p_index < 0 || p_index >= buffer_schemas.size()) return;

    godot::Ref<MemoryBufferResource> original = buffer_schemas[p_index];
    if (!original.is_valid()) return;

    // Deep-copy the configuration based on MemoryBufferResource properties[cite: 8]
    godot::Ref<MemoryBufferResource> duplicate_res;
    duplicate_res.instantiate();
    
    duplicate_res->set_buffer_name(original->get_buffer_name() + godot::StringName("_copy"));
    duplicate_res->set_layout_type(original->get_layout_type());
    duplicate_res->set_max_elements(original->get_max_elements());
    duplicate_res->set_alignment(original->get_alignment());
    duplicate_res->set_needs_gpu_compute(original->get_needs_gpu_compute());
    duplicate_res->set_enable_shadowing(original->get_enable_shadowing());
    duplicate_res->set_selection_mode(original->get_selection_mode());
    
    // Perform a deep copy of the columns dictionary array[cite: 7, 8]
    duplicate_res->set_columns(original->get_columns().duplicate(true)); 

    // Insert the duplicate immediately following the original
    int target_index = p_index + 1;

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Duplicate Memory Buffer");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_insert_buffer).bind(target_index, duplicate_res));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_remove_buffer).bind(target_index));
        undo_redo->commit_action();
    } else {
        _insert_buffer(target_index, duplicate_res);
    }
}

void MemoryManagerResource::move_buffers_bulk(const godot::PackedInt32Array& p_buffer_ids, int p_target_index) {
    if (p_buffer_ids.is_empty()) return;

    godot::TypedArray<MemoryBufferResource> remaining;
    godot::TypedArray<MemoryBufferResource> extracted;
    int insert_point = 0;

    // 1. Stable Partitioning: Separate the cut items from the remaining items
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        if (p_buffer_ids.has(i)) {
            extracted.push_back(buffer_schemas[i]);
        } else {
            remaining.push_back(buffer_schemas[i]);
            // Count how many "uncut" items fall before or exactly on our target index
            if (i <= p_target_index) {
                insert_point++;
            }
        }
    }

    // 2. Reconstruct the final array
    godot::TypedArray<MemoryBufferResource> new_schemas;
    for (int i = 0; i < insert_point; ++i) {
        new_schemas.push_back(remaining[i]);
    }
    for (int i = 0; i < extracted.size(); ++i) {
        new_schemas.push_back(extracted[i]);
    }
    for (int i = insert_point; i < remaining.size(); ++i) {
        new_schemas.push_back(remaining[i]);
    }

    // 3. Commit via the centralized schema setter for a clean, atomic Undo/Redo
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Move Buffers (Bulk)");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_buffer_schemas).bind(new_schemas));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_buffer_schemas).bind(buffer_schemas.duplicate()));
        undo_redo->commit_action();
    } else {
        _set_buffer_schemas(new_schemas);
    }
}

void MemoryManagerResource::duplicate_buffers_bulk(const godot::PackedInt32Array& p_buffer_ids, int p_target_index) {
    if (p_buffer_ids.is_empty()) return;

    godot::TypedArray<MemoryBufferResource> new_schemas;
    
    // Clamp target to prevent bounds issues if the list is empty or target is wild
    int safe_target = p_target_index;
    if (safe_target < 0) safe_target = -1; 
    if (safe_target >= buffer_schemas.size()) safe_target = buffer_schemas.size() - 1;

    // 1. Add original items up to and including the target index
    for (int i = 0; i <= safe_target; ++i) {
        new_schemas.push_back(buffer_schemas[i]);
    }

    // 2. Generate duplicates in their original relative order
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        if (p_buffer_ids.has(i)) {
            godot::Ref<MemoryBufferResource> original = buffer_schemas[i];
            if (!original.is_valid()) continue;

            godot::Ref<MemoryBufferResource> duplicate_res;
            duplicate_res.instantiate();
            
            duplicate_res->set_buffer_name(original->get_buffer_name() + godot::StringName("_copy"));
            duplicate_res->set_layout_type(original->get_layout_type());
            duplicate_res->set_max_elements(original->get_max_elements());
            duplicate_res->set_alignment(original->get_alignment());
            duplicate_res->set_needs_gpu_compute(original->get_needs_gpu_compute());
            duplicate_res->set_enable_shadowing(original->get_enable_shadowing());
            duplicate_res->set_selection_mode(original->get_selection_mode());
            duplicate_res->set_columns(original->get_columns().duplicate(true)); 

            new_schemas.push_back(duplicate_res);
        }
    }

    // 3. Add the remaining original items
    for (int i = safe_target + 1; i < buffer_schemas.size(); ++i) {
        new_schemas.push_back(buffer_schemas[i]);
    }

    // 4. Commit via atomic Undo/Redo
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Duplicate Buffers (Bulk)");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_buffer_schemas).bind(new_schemas));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_buffer_schemas).bind(buffer_schemas.duplicate()));
        undo_redo->commit_action();
    } else {
        _set_buffer_schemas(new_schemas);
    }
}

void MemoryManagerResource::set_scaling_strategy(int p_strategy) { 
    if (p_strategy == scaling_strategy) return;
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Set Scaling Strategy");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_scaling_strategy).bind(p_strategy));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_scaling_strategy).bind(scaling_strategy));
        undo_redo->commit_action();
    } else {
        _set_scaling_strategy(p_strategy);
    }
}
void MemoryManagerResource::_set_scaling_strategy(int p_strategy) { 
    scaling_strategy = static_cast<ScalabilityStrategy>(p_strategy); 
    emit_changed();
}
int MemoryManagerResource::get_scaling_strategy() const { return scaling_strategy; }

void MemoryManagerResource::set_transient_capacity_mb(int p_mb) { 
    if (p_mb == transient_capacity_mb) return;
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Set Transient Capacity");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_transient_capacity_mb).bind(p_mb));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_transient_capacity_mb).bind(transient_capacity_mb));
        undo_redo->commit_action();
    } else {
        _set_transient_capacity_mb(p_mb);
    }
}
void MemoryManagerResource::_set_transient_capacity_mb(int p_mb) { 
    transient_capacity_mb = p_mb; 
    emit_changed();
}
int MemoryManagerResource::get_transient_capacity_mb() const { return transient_capacity_mb; }


void MemoryManagerResource::set_managed_buffers(const godot::TypedArray<ManagedBufferResource>& p_buffers) { 
    if (p_buffers == managed_buffer_schemas) return;
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Set Managed Buffers");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_managed_buffers).bind(p_buffers.duplicate()));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_managed_buffers).bind(managed_buffer_schemas.duplicate()));
        undo_redo->commit_action();
    } else {
        _set_managed_buffers(p_buffers);
    }
}
void MemoryManagerResource::_set_managed_buffers(const godot::TypedArray<ManagedBufferResource>& p_buffers) { 
    managed_buffer_schemas = p_buffers; 
    emit_changed();
}
godot::TypedArray<ManagedBufferResource> MemoryManagerResource::get_managed_buffers() const { return managed_buffer_schemas; }

void MemoryManagerResource::move_managed_buffer(int p_from_index, int p_to_index) {
    if (p_from_index < 0 || p_from_index >= managed_buffer_schemas.size()) return;
    if (p_to_index < 0 || p_to_index >= managed_buffer_schemas.size()) return;
    if (p_from_index == p_to_index) return;

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Move Managed Profile");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_move_managed_buffer).bind(p_from_index, p_to_index));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_move_managed_buffer).bind(p_to_index, p_from_index));
        undo_redo->commit_action();
    } else {
        _move_managed_buffer(p_from_index, p_to_index);
    }
}

void MemoryManagerResource::_move_managed_buffer(int p_from_index, int p_to_index) {
    godot::Variant profile = managed_buffer_schemas[p_from_index];
    managed_buffer_schemas.remove_at(p_from_index);
    managed_buffer_schemas.insert(p_to_index, profile);
    emit_changed();
}

void MemoryManagerResource::set_active_emulated_grants(const godot::TypedArray<MemoryGrantResource>& p_grants) { 
    if (p_grants == active_emulated_grants) return;
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Set Active Emulated Grants");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_set_active_emulated_grants).bind(p_grants.duplicate()));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_active_emulated_grants).bind(active_emulated_grants.duplicate()));
        undo_redo->commit_action();
    } else {
        _set_active_emulated_grants(p_grants);
    }
}
void MemoryManagerResource::_set_active_emulated_grants(const godot::TypedArray<MemoryGrantResource>& p_grants) { 
    active_emulated_grants = p_grants;
    recalculate_emulated_grants();
    emit_changed(); 
}
godot::TypedArray<MemoryGrantResource> MemoryManagerResource::get_active_emulated_grants() const { return active_emulated_grants; }

void MemoryManagerResource::clear_consumer_buffers(const godot::StringName& p_consumer) {
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Clear Consumer Buffers");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_clear_consumer_buffers).bind(p_consumer));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_managed_buffers).bind(managed_buffer_schemas.duplicate()));
        undo_redo->commit_action();
    } else {
        _clear_consumer_buffers(p_consumer);
    }
}
void MemoryManagerResource::_clear_consumer_buffers(const godot::StringName& p_consumer) {
    bool changed = false;
    for (int i = managed_buffer_schemas.size() - 1; i >= 0; --i) {
        godot::Ref<ManagedBufferResource> consumer_buffer = managed_buffer_schemas[i];
        if (consumer_buffer->get_consumer_name() == p_consumer) {
            managed_buffer_schemas.remove_at(i);
            changed = true;
        }
    }
    if (changed) {
        emit_changed();
    }
}

void MemoryManagerResource::register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferResource>& p_buffers) {
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Register Consumer Buffers");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_register_consumer_buffers).bind(p_consumer, p_buffers.duplicate()));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_managed_buffers).bind(managed_buffer_schemas.duplicate()));
        undo_redo->commit_action();
    } else {
        _register_consumer_buffers(p_consumer, p_buffers);
    }
}
void MemoryManagerResource::_register_consumer_buffers(const godot::StringName& p_consumer, const godot::TypedArray<ManagedBufferResource>& p_buffers) {
    for (int i = managed_buffer_schemas.size() - 1; i >= 0; --i) {
        godot::Ref<ManagedBufferResource> consumer_buffer = managed_buffer_schemas[i];
        if (consumer_buffer->get_consumer_name() == p_consumer) {
            managed_buffer_schemas.remove_at(i);
        }
    }
    managed_buffer_schemas.append_array(p_buffers);
    emit_changed();
}

int MemoryManagerResource::get_total_projected_footprint_bytes() const {
    size_t simulated_offset = 0;

    // --- 1. Simulate Intrinsic Manager Overhead ---
    // MemoryManagerDOD::_initialize_system_buffers unconditionally creates the System Ring
    simulated_offset = core::MemoryUtilities::align_to(simulated_offset, 64);
    simulated_offset += 1024 * 1024; // 1 MB System Ring

    // --- 2. Simulate User Data Buffers ---
    // We must mimic initialize_backend()'s exact allocation math
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        godot::Ref<MemoryBufferResource> schema = buffer_schemas[i];
        if (!schema.is_valid()) continue;

        uint32_t buffer_align = static_cast<uint32_t>(schema->get_alignment());
        int64_t max_elems = schema->get_max_elements();

        godot::TypedArray<godot::Dictionary> dict_columns = schema->get_columns();
        size_t raw_data_size = 0;
        for (int j = 0; j < dict_columns.size(); ++j) {
            godot::Dictionary dict = dict_columns[j];
            uint32_t type_size = dict.has("size") ? static_cast<uint32_t>(dict["size"]) : 1;
            raw_data_size += (type_size * max_elems);
        }

        if (schema->get_enable_shadowing()) {
            // Shadowed Buffers consume TWO separate allocations.
            // 1. Primary Data Buffer (create_shadowed_buffer always forces 64-byte alignment here)
            simulated_offset = core::MemoryUtilities::align_to(simulated_offset, 64);
            simulated_offset += raw_data_size;

            // 2. The Shadow Metadata Buffer
            core::SelectionMode sel_mode = static_cast<core::SelectionMode>(schema->get_selection_mode());
            size_t selection_data_size = (sel_mode == core::SelectionMode::DENSE) ? 
                ((max_elems + 63) / 64) * sizeof(uint64_t) : max_elems * sizeof(int64_t);

            size_t meta_soa_size = (max_elems * sizeof(uint32_t)) + (max_elems * sizeof(int64_t)) + 
                       (max_elems * sizeof(uint8_t));
            
            simulated_offset = core::MemoryUtilities::align_to(simulated_offset, 64);
            simulated_offset += (selection_data_size + meta_soa_size + 256);
        } else {
            // Standard Buffer
            simulated_offset = core::MemoryUtilities::align_to(simulated_offset, buffer_align);
            simulated_offset += raw_data_size;
        }
    }

    // --- 3. Simulate Graph Consumer Profiles ---
    for (int i = 0; i < managed_buffer_schemas.size(); ++i) {
        godot::Ref<ManagedBufferResource> managed_buffer = managed_buffer_schemas[i];
        if (!managed_buffer.is_valid()) continue;
        
        uint32_t align = static_cast<uint32_t>(managed_buffer->get_alignment());
        size_t size = static_cast<size_t>(managed_buffer->get_byte_footprint());
        
        simulated_offset = core::MemoryUtilities::align_to(simulated_offset, align);
        simulated_offset += size;
    }

    return static_cast<int>(simulated_offset);
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

godot::Ref<MemoryGrantResource> MemoryManagerResource::request_emulated_grant(const godot::PackedInt32Array& p_buffer_ids) {
    using namespace godot;

    // 1. Instantiate the new parent MemoryGrantResource container
    Ref<MemoryGrantResource> new_grant;
    new_grant.instantiate();

    // Set a default descriptive tracking name for debugging/UI purposes
    new_grant->set_grant_name(String("Grant_") + String::num_int64(active_emulated_grants.size()));

    // 2. Build out the constituent GrantPartResource objects from the incoming array
    TypedArray<GrantPartResource> configured_parts;
    
    for (int i = 0; i < p_buffer_ids.size(); ++i) {
        int32_t target_buffer_id = p_buffer_ids[i];
        
        Ref<GrantPartResource> part_res;
        part_res.instantiate();
        
        // Populate the base configuration properties
        part_res->set_buffer_id(target_buffer_id);
        part_res->set_element_stride(0); // Default placeholder; will be finalized by schema verification
        part_res->set_access_mode(0);     // Default fallback to READ mode
        part_res->set_is_contiguous(true); 
        
        Ref<MemoryBufferSelectionResource> default_selection;
        default_selection.instantiate();
        part_res->set_selection(default_selection);

        if (target_buffer_id >= 0 && target_buffer_id < buffer_schemas.size()) {
            Ref<MemoryBufferResource> schema = buffer_schemas[target_buffer_id];
            if (schema.is_valid()) {
                part_res->set_buffer_type(schema->get_layout_type());
            }
        }

        configured_parts.push_back(part_res);
    }

    // Assign the generated array of parts directly into our new grant resource
    new_grant->set_configured_parts(configured_parts);

    // 3. Dry-Run Validation Loop
    // Temporarily add it to check for Unassigned Claims, Out of Bounds, or Race Conditions.
    active_emulated_grants.push_back(new_grant);
    recalculate_emulated_grants();

    // Context Evaluation
    // Check if the linter appended any errors to our newly minted grant during validation.
    if (!new_grant->is_emulated_valid()) { 
        UtilityFunctions::print_rich("[color=yellow]Memory Warning: Requested emulated grant failed linting validations.[/color]");
        UtilityFunctions::print_rich("[color=orange]Part Count: " + String::num_int64(new_grant->get_configured_parts().size()) + "[/color]");
        UtilityFunctions::print_rich("[color=orange]Capacity Mode: "  + String::num_int64(new_grant->get_capacity_mode()) + "[/color]");
        
        PackedStringArray errors = new_grant->get_emulation_errors();
        for (int i = 0; i < errors.size(); ++i) {
            UtilityFunctions::print_rich("[color=orange] - " + errors[i] + "[/color]");
        }
        
        // Failed systemic validation rules: cleanly revert simulation tracking pool
        int tracking_idx = active_emulated_grants.find(new_grant);
        if (tracking_idx != -1) {
            active_emulated_grants.remove_at(tracking_idx);
        }
        recalculate_emulated_grants();
        
        // Return a null pointer to signal compilation/configuration failure to the UI orchestrator
        return Ref<MemoryGrantResource>();
    }

    // 4. Clean Revert & Unified Commit
    // It's valid, but to support Undo/Redo tracking, we need to temporarily remove it 
    // from the raw array and let the UndoRedo pipeline explicitly run the `add` action.
    int tracking_idx = active_emulated_grants.find(new_grant);
    if (tracking_idx != -1) {
        active_emulated_grants.remove_at(tracking_idx);
    }
    recalculate_emulated_grants();

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Request Emulated Grant");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_add_emulated_grant).bind(new_grant));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_release_emulated_grant).bind(new_grant));
        undo_redo->commit_action(true);
    } else {
        _add_emulated_grant(new_grant);
    }

    // Success! Return the fully established, validated configuration token
    return new_grant;
}

void MemoryManagerResource::_add_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    active_emulated_grants.push_back(p_grant);
    recalculate_emulated_grants();
    emit_changed();
}

void MemoryManagerResource::release_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    if (!p_grant.is_valid()) return;
    
    int index = active_emulated_grants.find(p_grant);
    if (index == -1) return;

    if (undo_redo.is_valid()) {
        undo_redo->create_action("Release Emulated Grant");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_release_emulated_grant).bind(p_grant));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_insert_emulated_grant).bind(index, p_grant));
        undo_redo->commit_action();
    } else {
        _release_emulated_grant(p_grant);
    }
}

void MemoryManagerResource::_release_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    int index = active_emulated_grants.find(p_grant);
    if (index == -1) return;

    active_emulated_grants.remove_at(index);
    recalculate_emulated_grants();
    emit_changed();
}

void MemoryManagerResource::_insert_emulated_grant(int p_index, const godot::Ref<MemoryGrantResource>& p_grant) {
    if (p_index < 0 || p_index >= active_emulated_grants.size()) {
        active_emulated_grants.push_back(p_grant);
    } else {
        active_emulated_grants.insert(p_index, p_grant);
    }
    recalculate_emulated_grants();
    emit_changed();
}

void MemoryManagerResource::clear_all_emulated_grants() {
    if (active_emulated_grants.is_empty()) return;
    
    if (undo_redo.is_valid()) {
        undo_redo->create_action("Clear All Emulated Grants");
        undo_redo->add_do_method(callable_mp(this, &MemoryManagerResource::_clear_all_emulated_grants));
        undo_redo->add_undo_method(callable_mp(this, &MemoryManagerResource::_set_active_emulated_grants).bind(active_emulated_grants.duplicate()));
        undo_redo->commit_action();
    } else {
        _clear_all_emulated_grants();
    }
}

void MemoryManagerResource::_clear_all_emulated_grants() {
    active_emulated_grants.clear();
    recalculate_emulated_grants();
    emit_changed();
}

void MemoryManagerResource::recalculate_emulated_grants() {
    using namespace godot;

    // 1. Reset State across the entire dependency pool
    for (int i = 0; i < active_emulated_grants.size(); ++i) {
        Ref<MemoryGrantResource> grant = active_emulated_grants[i];
        if (grant.is_valid()) {
            grant->clear_emulation_state();
        }
    }

    // Structural structures needed to parse conflicts
    // Map tracking: Buffer ID -> Vector of structs tracking [Grant Object, Access Mode flag, Part Index]
    struct AccessorTrace {
        Ref<MemoryGrantResource> grant;
        int access_mode;
        int part_index;
    };
    std::map<uint32_t, std::vector<AccessorTrace>> structural_access_map;

    // 2. Pass 1: Local Integrity Linter (Examine constraints internal to each grant context)
    for (int i = 0; i < active_emulated_grants.size(); ++i) {
        Ref<MemoryGrantResource> grant = active_emulated_grants[i];
        if (!grant.is_valid()) continue;

        String name_ctx = grant->get_grant_name().is_empty() ? String("Grant ") + String::num_int64(i) : String("'") + grant->get_grant_name() + String("'");
        TypedArray<GrantPartResource> parts = grant->get_configured_parts();

        // Rule A: Enforce hard compiled bounds based on structural sizing configurations
        if (parts.size() > grant->get_capacity_mode()) {
            grant->add_emulation_error(
                String("Capacity Overflow: Layout contains ") + String::num_int64(parts.size()) + 
                " parts, but capacity mode limits allocations to exactly " + String::num_int64(grant->get_capacity_mode()) + " slots."
            );
        }

        // Evaluate child configurations
        for (int j = 0; j < parts.size(); ++j) {
            Ref<GrantPartResource> part = parts[j];
            if (!part.is_valid()) {
                grant->add_emulation_error(String("Null Reference Error: Configuration part element at index [") + String::num_int64(j) + "] is uninstantiated.");
                continue;
            }

            uint32_t target_id = part->get_buffer_id();

            // Rule B: Enforce structural existence bounds
            if (target_id == 0xFFFFFFFF) {
                grant->add_emulation_error(String("Unassigned Claim: Configuration part [") + String::num_int64(j) + "] has no assigned target Buffer ID.");
                continue;
            }

            // Verify if ID resolves to an active definition template inside schemas
            if (target_id >= static_cast<uint32_t>(buffer_schemas.size())) {
                grant->add_emulation_error(
                    String("Out of Bounds Reference: Part [") + String::num_int64(j) + 
                    "] points to Buffer ID " + String::num_int64(target_id) + 
                    ", but the manager only defines " + String::num_int64(buffer_schemas.size()) + " total schemas."
                );
                continue;
            }

            Ref<MemoryBufferResource> matching_schema = buffer_schemas[target_id];
            if (matching_schema.is_valid()) {
                // Synchronize the setup data strings automatically for easier graph node debugging
                part->set_target_buffer_name(matching_schema->get_buffer_name());
                
                // Rule C: Column metadata validations (Only if buffer acts as a columnar structure layout like SoA)
                int64_t layout_type = matching_schema->get_layout_type(); 
                if (layout_type == 2 /* core::BufferLayoutType::SOA */ || layout_type == 3 /* core::BufferLayoutType::SPARSE_SET */) {
                    TypedArray<Dictionary> cols = matching_schema->get_columns();
                    uint32_t col_id = part->get_column_id();
                    bool col_found = false;
                    
                    for (int c = 0; c < cols.size(); ++c) {
                        Dictionary d = cols[c];
                        uint32_t actual_cid = d.has("id") ? static_cast<uint32_t>(d["id"]) : static_cast<uint32_t>(c);
                        if (actual_cid == col_id) {
                            col_found = true;
                            if (d.has("name")) {
                                part->set_target_column_name(d["name"]);
                            }
                            break;
                        }
                    }
                    if (!col_found) {
                        grant->add_emulation_error(
                            String("Invalid Column Claim: Part [") + String::num_int64(j) + 
                            "] targets Column ID " + String::num_int64(col_id) + 
                            " within Buffer '" + matching_schema->get_buffer_name() + "', which does not exist."
                        );
                    }
                }
            }

            // Cache data vectors to perform full multi-grant concurrency checks next
            structural_access_map[target_id].push_back({ grant, part->get_access_mode(), j });
        }
    }

    // 3. Pass 2: Concurrent Graph Dependency Linter (Global cross-grant analysis)
    // Emulates memory_manager_dod's lock-free CAS validation engine rules
    for (auto const& [buffer_id, traces] : structural_access_map) {
        if (traces.size() <= 1) continue; // Single consumer allocations cannot conflict

        int write_count = 0;
        int read_count = 0;

        for (auto const& trace : traces) {
            if (trace.access_mode == 1) { // WRITE Access mode
                write_count++;
            } else {                      // READ Access mode
                read_count++;
            }
        }

        // Core Constraint Violation Rule: Exclusive Write Authority
        // High-performance thread workers will experience race conditions if a buffer 
        // has a writer shared with any other concurrent operations.
        if (write_count > 0) {
            Ref<MemoryBufferResource> schema = buffer_schemas[buffer_id];
            String b_name = schema.is_valid() ? 
                static_cast<String>(schema->get_buffer_name()) : 
                String("ID ") + String::num_int64(buffer_id);

            for (auto const& trace : traces) {
                String mode_str = (trace.access_mode == 1) ? "WRITE" : "READ";
                
                trace.grant->add_emulation_error(
                    String("Race Condition Collision: Part [") + String::num_int64(trace.part_index) + 
                    "] requests " + mode_str + " authorization on Buffer '" + b_name + 
                    "'. This fails validation because concurrent constraints dictate total system access contains (" + 
                    String::num_int64(write_count) + " Writers, " + String::num_int64(read_count) + " Readers)."
                );
            }
        }
    }
}

godot::Ref<MemoryGrantInspector> MemoryManagerResource::get_grant_inspector(int p_grant_index) const {
    godot::Ref<MemoryGrantInspector> grant_inspector;
    
    if (p_grant_index < 0 || p_grant_index >= active_emulated_grants.size()) {
        return grant_inspector; // Returns null
    }

    godot::Ref<MemoryGrantResource> grant = active_emulated_grants[p_grant_index];
    if (grant.is_null()) return grant_inspector;

    grant_inspector.instantiate();
    godot::TypedArray<godot::Dictionary> mock_parts;
    godot::TypedArray<GrantPartResource> parts = grant->get_configured_parts();

    for (int i = 0; i < parts.size(); ++i) {
        godot::Ref<GrantPartResource> part = parts[i];
        if (part.is_null()) continue;

        uint32_t b_id = part->get_buffer_id();
        int capacity = 0;

        // Contextual Lookup: Grab the capacity from the master schemas
        if (b_id >= 0 && b_id < buffer_schemas.size()) {
            godot::Ref<MemoryBufferResource> schema = buffer_schemas[b_id];
            if (schema.is_valid()) {
                capacity = schema->get_max_elements();
            }
        }

        godot::Dictionary dict;
        dict["buffer_id"] = b_id;
        dict["access_mode"] = part->get_access_mode() == 1 ? "WRITE" : "READ";
        dict["element_stride"] = part->get_element_stride();
        dict["capacity"] = capacity;

        // Construct and nest the Selection Inspector
        godot::Ref<MemoryBufferSelectionResource> sel_res = part->get_selection();
        godot::Ref<MemorySelectionInspector> sel_inspector;
        sel_inspector.instantiate();
        
        if (sel_res.is_valid()) {
            sel_inspector->setup_emulated_selection(sel_res, capacity, b_id);
            dict["element_count"] = sel_res->get_element_count();
        } else {
            dict["element_count"] = 0;
        }
        
        dict["selection"] = sel_inspector;
        mock_parts.push_back(dict);
    }

    grant_inspector->setup_emulated_grant(mock_parts);
    return grant_inspector;
}

void MemoryManagerResource::serialize_subresources_to_disk() {
    // 1. Ensure we only execute this logic inside the Godot editor
    if (!godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    // 2. Get the current path of the MemoryManagerResource
    godot::String current_path = get_path();
    if (current_path.is_empty() || !current_path.begins_with("res://")) {
        // The manager itself hasn't been saved to disk yet.
        // It must be saved first so we know where to create the sub-folder.
        return; 
    }

    // 3. Determine folder name and create it if it doesn't exist
    godot::String base_dir = current_path.get_base_dir();
    godot::String manager_name = get_name();
    if (manager_name.is_empty()) {
        manager_name = current_path.get_file().get_basename();
    }
    
    godot::String target_folder = base_dir.path_join(manager_name + "_resources");

    godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(base_dir);
    if (dir.is_valid() && !dir->dir_exists(target_folder)) {
        dir->make_dir(target_folder);
    }

    // 4. Lambda helper to save individual resources
    auto save_resource = [&](godot::Ref<godot::Resource> res, const godot::String& prefix, int index) {
        if (!res.is_valid()) return;
        
        // Use the resource's name if it has one, otherwise generate a fallback name
        godot::String res_name = res->get_name();
        if (res_name.is_empty()) {
            res_name = prefix + godot::String("_") + godot::String::num_int64(index);
        }
        
        // Construct the final path
        godot::String save_path = res->get_path();
        
        if (!save_path.is_empty()){
           save_path = target_folder.path_join(res_name + ".tres");
           res->set_path(save_path);
        }
        
        // Save to disk
        godot::ResourceSaver::get_singleton()->save(res, save_path);
    };

    // 5. Iterate through your arrays and save the sub-resources
    for (int i = 0; i < buffer_schemas.size(); ++i) {
        save_resource(buffer_schemas[i], "BufferSchema", i);
    }

    for (int i = 0; i < managed_buffer_schemas.size(); ++i) {
        save_resource(managed_buffer_schemas[i], "ManagedBufferSchema", i);
    }

    for (int i = 0; i < active_emulated_grants.size(); ++i) {
        save_resource(active_emulated_grants[i], "MemoryGrant", i);
    }
    
    // 6. Resave the MemoryManagerResource itself
    // This updates the manager's serialized data to point to the newly created external .tres files
    // instead of embedding them.
    godot::ResourceSaver::get_singleton()->save(this, current_path);
}


} // namespace ideam::godot_ext