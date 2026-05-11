#include "metadata_task_graph_node_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void MetadataTaskGraphNodeResource::_bind_methods() {
    // Grouping properties clearly defines the inspector bounds for designers
    // while keeping serialization strictly ordered.
    ADD_GROUP("Metadata Matrix Constraints", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_view_id", "id"), &MetadataTaskGraphNodeResource::set_view_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_view_id"), &MetadataTaskGraphNodeResource::get_view_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "view_id"), "set_view_id", "get_view_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_strategy_id", "id"), &MetadataTaskGraphNodeResource::set_strategy_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_strategy_id"), &MetadataTaskGraphNodeResource::get_strategy_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "strategy_id"), "set_strategy_id", "get_strategy_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_type_id", "id"), &MetadataTaskGraphNodeResource::set_type_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type_id"), &MetadataTaskGraphNodeResource::get_type_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type_id"), "set_type_id", "get_type_id");
}

void MetadataTaskGraphNodeResource::set_view_id(int p_id) {
    view_id = static_cast<uint32_t>(p_id);
}

int MetadataTaskGraphNodeResource::get_view_id() const {
    return static_cast<int>(view_id);
}

void MetadataTaskGraphNodeResource::set_strategy_id(int p_id) {
    strategy_id = static_cast<uint32_t>(p_id);
}

int MetadataTaskGraphNodeResource::get_strategy_id() const {
    return static_cast<int>(strategy_id);
}

void MetadataTaskGraphNodeResource::set_type_id(int p_id) {
    type_id = static_cast<uint32_t>(p_id);
}

int MetadataTaskGraphNodeResource::get_type_id() const {
    return static_cast<int>(type_id);
}

} // namespace ideam::godot_ext