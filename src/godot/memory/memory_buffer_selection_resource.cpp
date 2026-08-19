#include "memory_buffer_selection_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void MemoryBufferSelectionResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &MemoryBufferSelectionResource::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &MemoryBufferSelectionResource::get_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Sparse,Dense,Range"), "set_mode", "get_mode");

    ClassDB::bind_method(D_METHOD("set_element_count", "count"), &MemoryBufferSelectionResource::set_element_count);
    ClassDB::bind_method(D_METHOD("get_element_count"), &MemoryBufferSelectionResource::get_element_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "element_count"), "set_element_count", "get_element_count");

    ClassDB::bind_method(D_METHOD("set_start_index", "index"), &MemoryBufferSelectionResource::set_start_index);
    ClassDB::bind_method(D_METHOD("get_start_index"), &MemoryBufferSelectionResource::get_start_index);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "start_index"), "set_start_index", "get_start_index");

    ClassDB::bind_method(D_METHOD("set_sparse_indices", "indices"), &MemoryBufferSelectionResource::set_sparse_indices);
    ClassDB::bind_method(D_METHOD("get_sparse_indices"), &MemoryBufferSelectionResource::get_sparse_indices);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "sparse_indices"), "set_sparse_indices", "get_sparse_indices");

    ClassDB::bind_method(D_METHOD("set_dense_bitmask", "bitmask"), &MemoryBufferSelectionResource::set_dense_bitmask);
    ClassDB::bind_method(D_METHOD("get_dense_bitmask"), &MemoryBufferSelectionResource::get_dense_bitmask);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "dense_bitmask"), "set_dense_bitmask", "get_dense_bitmask");
}

void MemoryBufferSelectionResource::set_mode(int p_mode) {
    if (mode == p_mode) return;
    mode = static_cast<SelectionMode>(p_mode);
    emit_changed();
}
int MemoryBufferSelectionResource::get_mode() const { return mode; }

void MemoryBufferSelectionResource::set_element_count(int p_count) {
    if (element_count == p_count) return;
    element_count = p_count;
    emit_changed();
}
int MemoryBufferSelectionResource::get_element_count() const { return element_count; }

void MemoryBufferSelectionResource::set_start_index(int p_index) {
    if (start_index == p_index) return;
    start_index = p_index;
    emit_changed();
}
int MemoryBufferSelectionResource::get_start_index() const { return start_index; }

void MemoryBufferSelectionResource::set_sparse_indices(const godot::PackedInt64Array& p_indices) {
    if (sparse_indices == p_indices) return;
    sparse_indices = p_indices;
    emit_changed();
}
godot::PackedInt64Array MemoryBufferSelectionResource::get_sparse_indices() const { return sparse_indices; }

void MemoryBufferSelectionResource::set_dense_bitmask(const godot::PackedInt64Array& p_bitmask) {
    if (dense_bitmask == p_bitmask) return;
    dense_bitmask = p_bitmask;
    emit_changed();
}
godot::PackedInt64Array MemoryBufferSelectionResource::get_dense_bitmask() const { return dense_bitmask; }

} // namespace ideam::godot_ext