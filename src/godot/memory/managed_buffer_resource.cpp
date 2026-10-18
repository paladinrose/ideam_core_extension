#include "managed_buffer_resource.h"
#include "memory_manager_resource.h"
#include "../../core/memory/memory_manager_dod.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace ideam::godot_ext {

void ManagedBufferResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_consumer_name", "name"), &ManagedBufferResource::set_consumer_name);
    ClassDB::bind_method(D_METHOD("get_consumer_name"), &ManagedBufferResource::get_consumer_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "consumer_name"), "set_consumer_name", "get_consumer_name");

    ClassDB::bind_method(D_METHOD("set_purpose", "purpose"), &ManagedBufferResource::set_purpose);
    ClassDB::bind_method(D_METHOD("get_purpose"), &ManagedBufferResource::get_purpose);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "purpose"), "set_purpose", "get_purpose");

    ClassDB::bind_method(D_METHOD("set_schema_reference", "schema"), &ManagedBufferResource::set_schema_reference);
    ClassDB::bind_method(D_METHOD("get_schema_reference"), &ManagedBufferResource::get_schema_reference);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "schema_reference", PROPERTY_HINT_RESOURCE_TYPE, "MemoryBufferResource"), "set_schema_reference", "get_schema_reference");

    ClassDB::bind_method(D_METHOD("set_live_buffer_id", "id"), &ManagedBufferResource::set_live_buffer_id);
    ClassDB::bind_method(D_METHOD("get_live_buffer_id"), &ManagedBufferResource::get_live_buffer_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "live_buffer_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_live_buffer_id", "get_live_buffer_id");

    ClassDB::bind_method(D_METHOD("set_manager_resource", "manager"), &ManagedBufferResource::set_manager_resource);
    ClassDB::bind_method(D_METHOD("get_manager_resource"), &ManagedBufferResource::get_manager_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource", PROPERTY_USAGE_NONE), "set_manager_resource", "get_manager_resource");
    
    ClassDB::bind_method(D_METHOD("set_layout_type", "type"), &ManagedBufferResource::set_layout_type);
    ClassDB::bind_method(D_METHOD("get_layout_type"), &ManagedBufferResource::get_layout_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_type"), "set_layout_type", "get_layout_type");

    ClassDB::bind_method(D_METHOD("set_byte_footprint", "bytes"), &ManagedBufferResource::set_byte_footprint);
    ClassDB::bind_method(D_METHOD("get_byte_footprint"), &ManagedBufferResource::get_byte_footprint);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "byte_footprint"), "set_byte_footprint", "get_byte_footprint");

    ClassDB::bind_method(D_METHOD("set_alignment", "alignment"), &ManagedBufferResource::set_alignment);
    ClassDB::bind_method(D_METHOD("get_alignment"), &ManagedBufferResource::get_alignment);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment"), "set_alignment", "get_alignment");

    ClassDB::bind_method(D_METHOD("is_live"), &ManagedBufferResource::is_live);
    ClassDB::bind_method(D_METHOD("has_schema"), &ManagedBufferResource::has_schema);
    ClassDB::bind_method(D_METHOD("resolve_buffer_id"), &ManagedBufferResource::resolve_buffer_id);

    ClassDB::bind_method(D_METHOD("get_effective_buffer_name"), &ManagedBufferResource::get_effective_buffer_name);
    ClassDB::bind_method(D_METHOD("get_effective_layout_type"), &ManagedBufferResource::get_effective_layout_type);
    ClassDB::bind_method(D_METHOD("get_effective_schema"), &ManagedBufferResource::get_effective_schema);

    ClassDB::bind_method(D_METHOD("get_element_count"), &ManagedBufferResource::get_element_count);
    ClassDB::bind_method(D_METHOD("get_max_elements"), &ManagedBufferResource::get_max_elements);
    ClassDB::bind_method(D_METHOD("get_capacity_bytes"), &ManagedBufferResource::get_capacity_bytes);
    ClassDB::bind_method(D_METHOD("get_used_bytes"), &ManagedBufferResource::get_used_bytes);
    ClassDB::bind_method(D_METHOD("get_slack_bytes"), &ManagedBufferResource::get_slack_bytes);
    ClassDB::bind_method(D_METHOD("get_fragmentation_percentage"), &ManagedBufferResource::get_fragmentation_percentage);

    ClassDB::bind_method(D_METHOD("get_lifecycle_state"), &ManagedBufferResource::get_lifecycle_state);
    ClassDB::bind_method(D_METHOD("get_lifecycle_state_string"), &ManagedBufferResource::get_lifecycle_state_string);
    ClassDB::bind_method(D_METHOD("get_active_writer_count"), &ManagedBufferResource::get_active_writer_count);
    ClassDB::bind_method(D_METHOD("get_active_reader_count"), &ManagedBufferResource::get_active_reader_count);
    ClassDB::bind_method(D_METHOD("is_vram_dirty"), &ManagedBufferResource::is_vram_dirty);
}

ManagedBufferResource::ManagedBufferResource() {}

ManagedBufferResource::~ManagedBufferResource() {}

void ManagedBufferResource::set_consumer_name(const StringName& p_name) {
    if (consumer_name == p_name) return;
    consumer_name = p_name;
    emit_changed();
}
StringName ManagedBufferResource::get_consumer_name() const { return consumer_name; }

void ManagedBufferResource::set_purpose(const StringName& p_purpose) {
    if (purpose == p_purpose) return;
    purpose = p_purpose;
    emit_changed();
}
StringName ManagedBufferResource::get_purpose() const { return purpose; }

void ManagedBufferResource::set_schema_reference(const Ref<MemoryBufferResource>& p_schema) {
    if (schema_reference == p_schema) return;
    schema_reference = p_schema;
    synthetic_schema.unref();
    emit_changed();
}
Ref<MemoryBufferResource> ManagedBufferResource::get_schema_reference() const { return schema_reference; }

void ManagedBufferResource::set_live_buffer_id(int p_id) {
    if (live_buffer_id == p_id) return;
    live_buffer_id = p_id;
    synthetic_schema.unref();
    emit_changed();
}
int ManagedBufferResource::get_live_buffer_id() const { return live_buffer_id; }

void ManagedBufferResource::set_manager_resource(const Ref<MemoryManagerResource>& p_manager) {
    if (manager_resource == p_manager) return;
    manager_resource = p_manager;
    synthetic_schema.unref();
    emit_changed();
}
Ref<MemoryManagerResource> ManagedBufferResource::get_manager_resource() const { return manager_resource; }

void ManagedBufferResource::set_layout_type(int p_type) {
    if (fallback_layout_type == p_type) return;
    fallback_layout_type = p_type;
    emit_changed();
}
int ManagedBufferResource::get_layout_type() const { return fallback_layout_type; }

void ManagedBufferResource::set_byte_footprint(int p_bytes) {
    if (fallback_byte_footprint == p_bytes) return;
    fallback_byte_footprint = p_bytes;
    emit_changed();
}
int ManagedBufferResource::get_byte_footprint() const { return fallback_byte_footprint; }

void ManagedBufferResource::set_alignment(int p_alignment) {
    if (fallback_alignment == p_alignment) return;
    fallback_alignment = p_alignment;
    emit_changed();
}
int ManagedBufferResource::get_alignment() const { return fallback_alignment; }

int ManagedBufferResource::resolve_buffer_id() const {
    if (live_buffer_id >= 0) return live_buffer_id;

    if (schema_reference.is_valid() && manager_resource.is_valid()) {
        TypedArray<MemoryBufferResource> schemas = manager_resource->get_buffer_schemas();
        for (int i = 0; i < schemas.size(); ++i) {
            Ref<MemoryBufferResource> schema = schemas[i];
            if (schema == schema_reference) {
                // Buffer IDs are assigned consecutively starting after System Command Ring
                return i + 1;
            }
        }
    }
    return -1;
}

bool ManagedBufferResource::is_live() const {
    if (!manager_resource.is_valid() || !manager_resource->is_initialized()) return false;
    return resolve_buffer_id() >= 0;
}

bool ManagedBufferResource::has_schema() const {
    return schema_reference.is_valid();
}

ideam::core::MemoryBufferPOD* ManagedBufferResource::_get_live_pod() const {
    if (!is_live()) return nullptr;

    std::shared_ptr<core::MemoryManagerDOD> backend = manager_resource->get_backend();
    if (!backend) return nullptr;

    int id = resolve_buffer_id();
    return (id >= 0) ? backend->get_buffer(static_cast<uint32_t>(id)) : nullptr;
}

StringName ManagedBufferResource::get_effective_buffer_name() const {
    if (schema_reference.is_valid()) {
        return schema_reference->get_buffer_name();
    }
    if (!consumer_name.is_empty()) {
        return consumer_name;
    }
    int id = resolve_buffer_id();
    return (id >= 0) ? StringName(String("Buffer_") + String::num_int64(id)) : StringName("Unbound Buffer");
}

int ManagedBufferResource::get_effective_layout_type() const {
    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    if (pod) {
        return static_cast<int>(pod->layout_type);
    }
    if (schema_reference.is_valid()) {
        return schema_reference->get_layout_type();
    }
    return fallback_layout_type;
}

Ref<MemoryBufferResource> ManagedBufferResource::get_effective_schema() const {
    if (schema_reference.is_valid()) {
        return schema_reference;
    }

    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    if (!pod) return Ref<MemoryBufferResource>();

    if (synthetic_schema.is_valid() && cached_pod_version == pod->version) {
        return synthetic_schema;
    }

    synthetic_schema.instantiate();
    cached_pod_version = pod->version;

    synthetic_schema->set_buffer_name(get_effective_buffer_name());
    synthetic_schema->set_layout_type(static_cast<int>(pod->layout_type));
    synthetic_schema->set_alignment_mode(static_cast<int>(pod->alignment_mode));
    synthetic_schema->set_alignment(static_cast<int>(pod->alignment_requirement));
    synthetic_schema->set_max_elements(static_cast<int>(pod->max_elements));

    if (pod->layout_type == BufferLayoutType::TILED_SOA) {
        synthetic_schema->set_elements_per_tile(pod->extra.tiled.elements_per_tile);
    } else if (pod->layout_type == BufferLayoutType::PAGED) {
        synthetic_schema->set_page_size_bytes(pod->extra.paged.page_size_bytes);
    }

    TypedArray<Dictionary> cols;
    for (uint32_t c = 0; c < pod->column_count; ++c) {
        const core::ColumnMetadata& col_meta = pod->columns[c];
        Dictionary col;
        col["id"] = col_meta.id;
        col["name"] = String("col_") + String::num_int64(col_meta.id);
        col["data_type"] = static_cast<int64_t>(col_meta.data_type);
        col["size"] = static_cast<int>(col_meta.type_size);
        col["alignment"] = static_cast<int>(col_meta.alignment);
        cols.push_back(col);
    }
    synthetic_schema->set_columns(cols);

    return synthetic_schema;
}

int64_t ManagedBufferResource::get_element_count() const {
    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    return pod ? pod->current_count : 0;
}

int64_t ManagedBufferResource::get_max_elements() const {
    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    if (pod) return pod->max_elements;
    if (schema_reference.is_valid()) return schema_reference->get_max_elements();
    return 0;
}

int64_t ManagedBufferResource::get_capacity_bytes() const {
    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    if (pod) return static_cast<int64_t>(pod->capacity_bytes);
    if (schema_reference.is_valid()) return schema_reference->calculate_raw_data_bytes();
    return fallback_byte_footprint;
}

int64_t ManagedBufferResource::get_used_bytes() const {
    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    if (pod) return static_cast<int64_t>(pod->used_bytes);
    return 0;
}

int64_t ManagedBufferResource::get_slack_bytes() const {
    int64_t cap = get_capacity_bytes();
    int64_t used = get_used_bytes();
    return (cap >= used) ? (cap - used) : 0;
}

double ManagedBufferResource::get_fragmentation_percentage() const {
    int64_t cap = get_capacity_bytes();
    if (cap <= 0) return 0.0;
    int64_t slack = get_slack_bytes();
    return (static_cast<double>(slack) / static_cast<double>(cap)) * 100.0;
}

int ManagedBufferResource::get_lifecycle_state() const {
    const ideam::core::MemoryBufferPOD* pod = _get_live_pod();
    return pod ? static_cast<int>(pod->lifecycle) : static_cast<int>(BufferLifecycleState::IDLE);
}

String ManagedBufferResource::get_lifecycle_state_string() const {
    switch (get_lifecycle_state()) {
        case 0:  return "IDLE";
        case 1:  return "LOCKED";
        case 2:  return "MIGRATING";
        default: return "UNKNOWN";
    }
}

int ManagedBufferResource::get_active_writer_count() const {
    if (!manager_resource.is_valid()) return 0;
    int target_id = resolve_buffer_id();
    if (target_id < 0) return 0;

    int writers = 0;
    TypedArray<MemoryGrantResource> grants = manager_resource->get_active_emulated_grants();
    for (int i = 0; i < grants.size(); ++i) {
        Ref<MemoryGrantResource> grant = grants[i];
        if (!grant.is_valid()) continue;

        TypedArray<GrantPartResource> parts = grant->get_configured_parts();
        for (int p = 0; p < parts.size(); ++p) {
            Ref<GrantPartResource> part = parts[p];
            if (part.is_valid() && part->get_buffer_id() == static_cast<uint32_t>(target_id) && part->get_access_mode() == 1) {
                writers++;
            }
        }
    }
    return writers;
}

int ManagedBufferResource::get_active_reader_count() const {
    if (!manager_resource.is_valid()) return 0;
    int target_id = resolve_buffer_id();
    if (target_id < 0) return 0;

    int readers = 0;
    TypedArray<MemoryGrantResource> grants = manager_resource->get_active_emulated_grants();
    for (int i = 0; i < grants.size(); ++i) {
        Ref<MemoryGrantResource> grant = grants[i];
        if (!grant.is_valid()) continue;

        TypedArray<GrantPartResource> parts = grant->get_configured_parts();
        for (int p = 0; p < parts.size(); ++p) {
            Ref<GrantPartResource> part = parts[p];
            if (part.is_valid() && part->get_buffer_id() == static_cast<uint32_t>(target_id) && part->get_access_mode() == 0) {
                readers++;
            }
        }
    }
    return readers;
}

bool ManagedBufferResource::is_vram_dirty() const {
    return false; // Handled through manager GPU sync commands
}

} // namespace ideam::godot_ext