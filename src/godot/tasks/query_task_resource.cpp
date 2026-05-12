#include "query_task_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void QueryTaskResource::_bind_methods() {
    // Explicit UI grouping for the 4D topological constraints
    ADD_GROUP("Query Matrix Constraints", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_op_id", "id"), &QueryTaskResource::set_op_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_op_id"), &QueryTaskResource::get_op_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "op_id"), "set_op_id", "get_op_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_view_id", "id"), &QueryTaskResource::set_view_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_view_id"), &QueryTaskResource::get_view_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "view_id"), "set_view_id", "get_view_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_strategy_id", "id"), &QueryTaskResource::set_strategy_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_strategy_id"), &QueryTaskResource::get_strategy_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "strategy_id"), "set_strategy_id", "get_strategy_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_type_id", "id"), &QueryTaskResource::set_type_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type_id"), &QueryTaskResource::get_type_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type_id"), "set_type_id", "get_type_id");
}

void QueryTaskResource::set_op_id(int p_id) {
    op_id = static_cast<uint32_t>(p_id);
}

int QueryTaskResource::get_op_id() const {
    return static_cast<int>(op_id);
}

void QueryTaskResource::set_view_id(int p_id) {
    view_id = static_cast<uint32_t>(p_id);
}

int QueryTaskResource::get_view_id() const {
    return static_cast<int>(view_id);
}

void QueryTaskResource::set_strategy_id(int p_id) {
    strategy_id = static_cast<uint32_t>(p_id);
}

int QueryTaskResource::get_strategy_id() const {
    return static_cast<int>(strategy_id);
}

void QueryTaskResource::set_type_id(int p_id) {
    type_id = static_cast<uint32_t>(p_id);
}

int QueryTaskResource::get_type_id() const {
    return static_cast<int>(type_id);
}

} // namespace ideam::godot_ext