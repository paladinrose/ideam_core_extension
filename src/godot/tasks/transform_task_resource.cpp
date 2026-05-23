#include "transform_task_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void TransformTaskResource::_bind_methods() {
    // Explicit UI grouping for the 3D topological constraints
    ADD_GROUP("Transform Matrix Constraints", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_view_id", "id"), &TransformTaskResource::set_view_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_view_id"), &TransformTaskResource::get_view_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "view_id"), "set_view_id", "get_view_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_strategy_id", "id"), &TransformTaskResource::set_strategy_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_strategy_id"), &TransformTaskResource::get_strategy_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "strategy_id"), "set_strategy_id", "get_strategy_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_type_id", "id"), &TransformTaskResource::set_type_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_type_id"), &TransformTaskResource::get_type_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "type_id"), "set_type_id", "get_type_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_logic_id", "logic_id"), &TransformTaskResource::set_logic_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_logic_id"), &TransformTaskResource::get_logic_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "logic_id"), "set_logic_id", "get_logic_id"); 

}

void TransformTaskResource::set_view_id(int p_id) {
    view_id = static_cast<uint32_t>(p_id);
}

int TransformTaskResource::get_view_id() const {
    return static_cast<int>(view_id);
}

void TransformTaskResource::set_strategy_id(int p_id) {
    strategy_id = static_cast<uint32_t>(p_id);
}

int TransformTaskResource::get_strategy_id() const {
    return static_cast<int>(strategy_id);
}

void TransformTaskResource::set_type_id(int p_id) {
    type_id = static_cast<uint32_t>(p_id);
}

int TransformTaskResource::get_type_id() const {
    return static_cast<int>(type_id);
}

void TransformTaskResource::set_logic_id(uint32_t p_id) {
    logic_id = p_id;
}

uint32_t TransformTaskResource::get_logic_id() const {
    return logic_id;
}

} // namespace ideam::godot_ext