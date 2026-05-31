#include "task_manifest.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::core {

void TaskManifest::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_query_matrix", "p_matrix"), &TaskManifest::set_query_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_query_matrix"), &TaskManifest::get_query_matrix);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "query_matrix"), "set_query_matrix", "get_query_matrix");

    godot::ClassDB::bind_method(godot::D_METHOD("set_transform_matrix", "p_matrix"), &TaskManifest::set_transform_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_transform_matrix"), &TaskManifest::get_transform_matrix);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "transform_matrix"), "set_transform_matrix", "get_transform_matrix");

    godot::ClassDB::bind_method(godot::D_METHOD("set_metadata_matrix", "p_matrix"), &TaskManifest::set_metadata_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_metadata_matrix"), &TaskManifest::get_metadata_matrix);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "metadata_matrix"), "set_metadata_matrix", "get_metadata_matrix");

    godot::ClassDB::bind_method(godot::D_METHOD("set_utility_matrix", "p_matrix"), &TaskManifest::set_utility_matrix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_utility_matrix"), &TaskManifest::get_utility_matrix);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "utility_matrix"), "set_utility_matrix", "get_utility_matrix");

    godot::ClassDB::bind_method(godot::D_METHOD("set_manifest_version", "p_version"), &TaskManifest::set_manifest_version);
    godot::ClassDB::bind_method(godot::D_METHOD("get_manifest_version"), &TaskManifest::get_manifest_version);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "manifest_version"), "set_manifest_version", "get_manifest_version");
}

} // namespace ideam::core