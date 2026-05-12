#include "sub_graph_task_resource.h"
// #include "task_graph_resource.h" // Ensure this is included in your actual build environment
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void SubGraphTaskResource::_bind_methods() {
    // Grouping UI properties for hierarchical topology logic
    ADD_GROUP("Sub-Graph Constraints", "");

    // Child Graph Reference
    godot::ClassDB::bind_method(godot::D_METHOD("set_child_graph", "graph"), &SubGraphTaskResource::set_child_graph);
    godot::ClassDB::bind_method(godot::D_METHOD("get_child_graph"), &SubGraphTaskResource::get_child_graph);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "child_graph", godot::PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"), "set_child_graph", "get_child_graph");

    // Memory Aliasing Mappings
    godot::ClassDB::bind_method(godot::D_METHOD("set_grant_mappings", "mappings"), &SubGraphTaskResource::set_grant_mappings);
    godot::ClassDB::bind_method(godot::D_METHOD("get_grant_mappings"), &SubGraphTaskResource::get_grant_mappings);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "grant_mappings"), "set_grant_mappings", "get_grant_mappings");
}

void SubGraphTaskResource::set_child_graph(const godot::Ref<TaskGraphResource>& p_graph) {
    child_graph = p_graph;
}

godot::Ref<TaskGraphResource> SubGraphTaskResource::get_child_graph() const {
    return child_graph;
}

void SubGraphTaskResource::set_grant_mappings(const godot::Dictionary& p_mappings) {
    grant_mappings = p_mappings;
}

godot::Dictionary SubGraphTaskResource::get_grant_mappings() const {
    return grant_mappings;
}

} // namespace ideam::godot_ext