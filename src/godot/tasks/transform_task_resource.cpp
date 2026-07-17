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

}

void TransformTaskResource::set_view_id(int p_id) {
    if (p_id == view_id) return;
    view_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int TransformTaskResource::get_view_id() const { return static_cast<int>(view_id); }

void TransformTaskResource::set_strategy_id(int p_id) {
    if (p_id == strategy_id) return;
    strategy_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int TransformTaskResource::get_strategy_id() const { return static_cast<int>(strategy_id); }

void TransformTaskResource::set_type_id(int p_id) {
    if (p_id == type_id) return;
    type_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int TransformTaskResource::get_type_id() const { return static_cast<int>(type_id); }

} // namespace ideam::godot_ext