#include "entry_fill_task_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void EntryFillTaskResource::_bind_methods() {
    // Explicit UI grouping for Data-Oriented graph entry constraints
    ADD_GROUP("Entry Constraints", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_buffer_id", "id"), &EntryFillTaskResource::set_target_buffer_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_buffer_id"), &EntryFillTaskResource::get_target_buffer_id);
    
    // Exposed as an INT to match Godot's Variant constraints, but safely backed by our uint32_t memory model
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "target_buffer_id"), "set_target_buffer_id", "get_target_buffer_id");
}

void EntryFillTaskResource::set_target_buffer_id(int p_id) {
    target_buffer_id = static_cast<uint32_t>(p_id);
}

int EntryFillTaskResource::get_target_buffer_id() const {
    return static_cast<int>(target_buffer_id);
}

} // namespace ideam::godot_ext