#include "task_graph_node_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void TaskGraphNodeResource::_bind_methods() {
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

bool TaskGraphNodeResource::validate_for_compilation() const {
    // 1. Pruning Phase Check
    if (!is_active) {
        // While not structurally "invalid", an inactive node should signal 
        // the graph compiler to skip allocating a POD command for it.
        // Depending on your compiler implementation, returning false here 
        // serves as an early-out prune. If you handle pruning separately,
        // you can omit this check.
        return false;
    }

    // 2. Cascade Up the Hierarchy
    // Validates Memory Grants (cache boundaries) and Ideam Graph Identity (topology)
    if (!MemoryGraphNodeResource::validate_for_compilation()) {
        return false;
    }

    return true;
}

} // namespace ideam::godot_ext