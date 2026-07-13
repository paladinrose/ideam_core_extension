#include "memory_manager_resource.h"
#include "../../core/memory/memory_common.h"
#include "../../core/memory/selection_utils.h" // Needed for core::SelectionMode if it's declared there

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>
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
    
    ClassDB::bind_method(D_METHOD("request_emulated_grant", "buffer_ids"), &MemoryManagerResource::request_emulated_grant);
    ClassDB::bind_method(D_METHOD("release_emulated_grant", "grant"), &MemoryManagerResource::release_emulated_grant);
    ClassDB::bind_method(D_METHOD("clear_all_emulated_grants"), &MemoryManagerResource::clear_all_emulated_grants);
    ClassDB::bind_method(D_METHOD("recalculate_emulated_grants"), &MemoryManagerResource::recalculate_emulated_grants);
    
    ClassDB::bind_method(D_METHOD("get_active_emulated_grants"), &MemoryManagerResource::get_active_emulated_grants);
    ClassDB::bind_method(D_METHOD("set_active_emulated_grants", "grants"), &MemoryManagerResource::set_active_emulated_grants);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "active_emulated_grants", PROPERTY_HINT_ARRAY_TYPE, "MemoryGrantResource"), "set_active_emulated_grants", "get_active_emulated_grants");
}

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
                                   (max_elems * sizeof(uint32_t)) + (max_elems * sizeof(uint8_t));

            simulated_offset = core::MemoryUtilities::align_to(simulated_offset, 64);
            simulated_offset += (selection_data_size + meta_soa_size + 256);
        } else {
            // Standard Buffer
            simulated_offset = core::MemoryUtilities::align_to(simulated_offset, buffer_align);
            simulated_offset += raw_data_size;
        }
    }

    // --- 3. Simulate Graph Consumer Profiles ---
    for (int i = 0; i < managed_profiles.size(); ++i) {
        godot::Ref<ManagedBufferProfile> profile = managed_profiles[i];
        if (!profile.is_valid()) continue;
        
        uint32_t align = static_cast<uint32_t>(profile->get_alignment());
        size_t size = static_cast<size_t>(profile->get_byte_footprint());
        
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

    // 3. Register our fresh grant into the global manager tracking pool
    active_emulated_grants.push_back(new_grant);

    // 4. Run the authoritative validation linter loop
    // This populates any "Unassigned Claims", "Out of Bounds", or "Race Condition Collisions".
    recalculate_emulated_grants();

    // 5. Contextual Context Evaluation
    // Check if the linter appended any errors to our newly minted grant during validation.
    if (!new_grant->is_emulated_valid()) { 
        UtilityFunctions::print_rich("[color=yellow]Memory Warning: Requested emulated grant failed linting validations.[/color]");
        UtilityFunctions::print_rich("[color=orange]Part Count: " + String::num_int64(new_grant->get_configured_parts().size()) + "[/color]");
        UtilityFunctions::print_rich("[color=orange]Capacity Mode: "  + String::num_int64(new_grant->get_capacity_mode()) + "[/color]");
        
        PackedStringArray errors = new_grant->get_emulation_errors();
        for (int i = 0; i < errors.size(); ++i) {
            UtilityFunctions::print_rich("[color=orange] - " + errors[i] + "[/color]");
        }
        // If it failed systemic validation rules, remove it from our active simulation tracking pool
        int tracking_idx = active_emulated_grants.find(new_grant);
        if (tracking_idx != -1) {
            active_emulated_grants.remove_at(tracking_idx);
        }
        
        // Return a null pointer to signal compilation/configuration failure to the UI orchestrator
        return Ref<MemoryGrantResource>();
    }

    // Success! Return the fully established, validated configuration token
    return new_grant;
}

void MemoryManagerResource::release_emulated_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    if (!p_grant.is_valid()) return;
    
    int index = active_emulated_grants.find(p_grant);
    if (index != -1) {
        active_emulated_grants.remove_at(index);
    }
    recalculate_emulated_grants();
}

void MemoryManagerResource::clear_all_emulated_grants() {
    active_emulated_grants.clear();
    recalculate_emulated_grants();
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

} // namespace ideam::godot_ext