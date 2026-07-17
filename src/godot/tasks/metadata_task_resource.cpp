#include "metadata_task_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void MetadataTaskResource::_bind_methods() {
    // Grouping properties clearly defines the inspector bounds for designers
    // while keeping serialization strictly ordered.
    ADD_GROUP("Metadata Matrix Constraints", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_view_id", "id"), &MetadataTaskResource::set_view_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_view_id"), &MetadataTaskResource::get_view_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "view_id"), "set_view_id", "get_view_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_strategy_id", "id"), &MetadataTaskResource::set_strategy_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_strategy_id"), &MetadataTaskResource::get_strategy_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "strategy_id"), "set_strategy_id", "get_strategy_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_type_id", "id"), &MetadataTaskResource::set_type_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type_id"), &MetadataTaskResource::get_type_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type_id"), "set_type_id", "get_type_id");

}

void MetadataTaskResource::set_view_id(int p_id) {
    if (p_id == view_id) return;
    view_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int MetadataTaskResource::get_view_id() const { return static_cast<int>(view_id); }

void MetadataTaskResource::set_strategy_id(int p_id) {
    if (p_id == strategy_id) return;
    strategy_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int MetadataTaskResource::get_strategy_id() const { return static_cast<int>(strategy_id); }

void MetadataTaskResource::set_type_id(int p_id) {
    if (p_id == type_id) return;
    type_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int MetadataTaskResource::get_type_id() const { return static_cast<int>(type_id); }

} // namespace ideam::godot_ext