#include "task_graph_node_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void TaskGraphNodeResource::_bind_methods() {
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_GODOT_REFLECTION", TASK_GODOT_REFLECTION);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_NATIVE_CPU", TASK_NATIVE_CPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_COMPUTE_GPU", TASK_COMPUTE_GPU);
    godot::ClassDB::bind_integer_constant(get_class_static(), "TaskType", "TASK_QUERY_CULLER", TASK_QUERY_CULLER);

    godot::ClassDB::bind_integer_constant(get_class_static(), "ExecutionMode", "EXEC_SERIAL", EXEC_SERIAL);
    godot::ClassDB::bind_integer_constant(get_class_static(), "ExecutionMode", "EXEC_PARALLEL", EXEC_PARALLEL);

    // Group scheduling variables distinctly from Visuals and Memory Constraints
    ADD_GROUP("Task Execution", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_is_active", "active"), &TaskGraphNodeResource::set_is_active);
    godot::ClassDB::bind_method(godot::D_METHOD("get_is_active"), &TaskGraphNodeResource::get_is_active);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "is_active"), "set_is_active", "get_is_active");

    godot::ClassDB::bind_method(godot::D_METHOD("set_execution_mode", "mode"), &TaskGraphNodeResource::set_execution_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_execution_mode"), &TaskGraphNodeResource::get_execution_mode);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "execution_mode", godot::PROPERTY_HINT_ENUM, "Serial,Parallel"), "set_execution_mode", "get_execution_mode");

    godot::ClassDB::bind_method(godot::D_METHOD("set_task_type", "type"), &TaskGraphNodeResource::set_task_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_type"), &TaskGraphNodeResource::get_task_type);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "task_type", godot::PROPERTY_HINT_ENUM, "Godot Reflection,Native CPU,Compute GPU,Query Culler"), "set_task_type", "get_task_type");

    godot::ClassDB::bind_method(godot::D_METHOD("set_task_properties", "props"), &TaskGraphNodeResource::set_task_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_properties"), &TaskGraphNodeResource::get_task_properties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "task_properties"), "set_task_properties", "get_task_properties");

    godot::ClassDB::bind_method(godot::D_METHOD("validate_for_compilation"), &TaskGraphNodeResource::validate_for_compilation);
}

void TaskGraphNodeResource::set_execution_mode(int p_mode) {
    exec_mode = static_cast<ExecutionMode>(p_mode);
}

int TaskGraphNodeResource::get_execution_mode() const {
    return static_cast<int>(exec_mode);
}

void TaskGraphNodeResource::set_is_active(bool p_active) {
    is_active = p_active;
}

bool TaskGraphNodeResource::get_is_active() const {
    return is_active;
}

void TaskGraphNodeResource::set_task_type(int p_type) {
    task_type = static_cast<TaskType>(p_type);
}

int TaskGraphNodeResource::get_task_type() const {
    return static_cast<int>(task_type);
}

void TaskGraphNodeResource::set_task_properties(const godot::Dictionary& p_props) {
    task_properties = p_props;
}

godot::Dictionary TaskGraphNodeResource::get_task_properties() const {
    return task_properties;
}

bool TaskGraphNodeResource::validate_for_compilation() const {
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

} // namespace ideam::godot_ext