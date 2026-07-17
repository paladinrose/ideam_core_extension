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
    if (p_id == target_buffer_id) return;
    target_buffer_id = static_cast<uint32_t>(p_id);
    emit_changed();
}
int EntryFillTaskResource::get_target_buffer_id() const { return static_cast<int>(target_buffer_id); }

godot::Dictionary EntryFillTaskResource::get_task_properties() const {
    godot::Dictionary props;
    
    // Explicit 64-bit cast to align with Variant::INT expectations,
    // avoiding unintended sign-extension anomalies from the 32-bit unsigned source.
    props["task_name"] = "EntryFillTask";
    props["target_buffer_id"] = static_cast<int64_t>(target_buffer_id);
    
    return props;
}

} // namespace ideam::godot_ext