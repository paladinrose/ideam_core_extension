#include "task_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void TaskResource::_bind_methods() {
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_GODOT_REFLECTION", TASK_GODOT_REFLECTION);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_NATIVE_CPU", TASK_NATIVE_CPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_COMPUTE_GPU", TASK_COMPUTE_GPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_QUERY_CULLER", TASK_QUERY_CULLER);

    godot::ClassDB::bind_integer_constant(get_class_static(), "ExecutionMode", "EXEC_SERIAL", EXEC_SERIAL);
    godot::ClassDB::bind_integer_constant(get_class_static(), "ExecutionMode", "EXEC_PARALLEL", EXEC_PARALLEL);

    // Group scheduling variables distinctly from Visuals and Memory Constraints
    ADD_GROUP("Task Execution", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_is_active", "active"), &TaskResource::set_is_active);
    godot::ClassDB::bind_method(godot::D_METHOD("get_is_active"), &TaskResource::get_is_active);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "is_active"), "set_is_active", "get_is_active");

    godot::ClassDB::bind_method(godot::D_METHOD("set_execution_mode", "mode"), &TaskResource::set_execution_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_execution_mode"), &TaskResource::get_execution_mode);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "execution_mode", godot::PROPERTY_HINT_ENUM, "Serial,Parallel"), "set_execution_mode", "get_execution_mode");

    godot::ClassDB::bind_method(godot::D_METHOD("set_task_type", "type"), &TaskResource::set_task_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_type"), &TaskResource::get_task_type);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "task_type", godot::PROPERTY_HINT_ENUM, "Godot Reflection,Native CPU,Compute GPU,Query Culler"), "set_task_type", "get_task_type");

    godot::ClassDB::bind_method(godot::D_METHOD("set_task_name", "name"), &TaskResource::set_task_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_name"), &TaskResource::get_task_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "task_name"), "set_task_name", "get_task_name");  
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_task_properties", "props"), &TaskResource::set_task_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_properties"), &TaskResource::get_task_properties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "task_properties"), "set_task_properties", "get_task_properties");

    godot::ClassDB::bind_method(godot::D_METHOD("set_logic_id", "logic_id"), &TaskResource::set_logic_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_logic_id"), &TaskResource::get_logic_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "logic_id"), "set_logic_id", "get_logic_id"); 

    godot::ClassDB::bind_method(godot::D_METHOD("set_base_logic_id", "base_logic_id"), &TaskResource::set_base_logic_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_base_logic_id"), &TaskResource::get_base_logic_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "base_logic_id"), "set_base_logic_id", "get_base_logic_id");
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_logic_variant_labels", "labels"), &TaskResource::set_logic_variant_labels);
    godot::ClassDB::bind_method(godot::D_METHOD("get_logic_variant_labels"), &TaskResource::get_logic_variant_labels);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::PACKED_STRING_ARRAY, "logic_variant_labels"), "set_logic_variant_labels", "get_logic_variant_labels"); 

    godot::ClassDB::bind_method(godot::D_METHOD("validate_for_compilation"), &TaskResource::validate_for_compilation);
}

void TaskResource::set_execution_mode(int p_mode) {
    if (p_mode == exec_mode) return;
    exec_mode = static_cast<ExecutionMode>(p_mode);
    emit_changed();
}
int TaskResource::get_execution_mode() const { return static_cast<int>(exec_mode); }

void TaskResource::set_is_active(bool p_active) {
    if(p_active == is_active) return;
    is_active = p_active;
    emit_changed();
}
bool TaskResource::get_is_active() const { return is_active; }

void TaskResource::set_task_type(int p_type) {
    if (p_type == task_type) return;
    task_type = static_cast<TaskType>(p_type);
    emit_changed();
}
int TaskResource::get_task_type() const { return static_cast<int>(task_type); }

void TaskResource::set_task_name(const godot::StringName& p_name) {
    if (p_name == task_name) return;
    task_name = p_name;
    emit_changed();
}
godot::StringName TaskResource::get_task_name() const { return task_name; }

void TaskResource::set_task_properties(const godot::Dictionary& p_props) {
    if (p_props == task_properties) return;
    task_properties = p_props.duplicate(); 
    emit_changed();
}

godot::Dictionary TaskResource::get_task_properties() const {
    return task_properties;
}

bool TaskResource::validate_for_compilation() const {
    // 1. Pruning Phase Check
    if (!is_active) {
        return false;
    }

    // 2. Cascade Up the Hierarchy
    if (!MemoryGraphNodeResource::validate_for_compilation()) {
        return false;
    }

    return true;
}

void TaskResource::set_logic_id(uint32_t p_id) {
    if(p_id == logic_id) return;
    logic_id = p_id;
    emit_changed();
}
uint32_t TaskResource::get_logic_id() const { return logic_id; }

void TaskResource::set_base_logic_id(uint32_t p_id) {
    if (p_id == base_logic_id) return;
    base_logic_id = p_id;
    emit_changed();
}
uint32_t TaskResource::get_base_logic_id() const { return base_logic_id; }

void TaskResource::set_logic_variant_labels(const godot::PackedStringArray& p_labels) {
    if (p_labels == logic_variant_labels) return;
    logic_variant_labels = p_labels;
    emit_changed();
}
godot::PackedStringArray TaskResource::get_logic_variant_labels() const { return logic_variant_labels; }

} // namespace ideam::godot_ext