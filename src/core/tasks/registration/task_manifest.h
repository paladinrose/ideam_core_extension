#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::core {

class TaskManifest : public godot::Resource {
    GDCLASS(TaskManifest, godot::Resource)

private:
    godot::Dictionary query_matrix;
    godot::Dictionary transform_matrix;
    godot::Dictionary metadata_matrix;
    godot::Dictionary utility_matrix;
    int manifest_version = 0;

protected:
    static void _bind_methods();

public:
    static constexpr int CURRENT_MANIFEST_VERSION = 1;
    static constexpr const char* MANIFEST_PATH = "res://addons/ideam_tasks/resources/task_manifest.tres";

    TaskManifest() = default;
    ~TaskManifest() = default;

    // --- Serialization Getters/Setters ---
    void set_query_matrix(const godot::Dictionary& p_matrix) { query_matrix = p_matrix; }
    godot::Dictionary get_query_matrix() const { return query_matrix; }

    void set_transform_matrix(const godot::Dictionary& p_matrix) { transform_matrix = p_matrix; }
    godot::Dictionary get_transform_matrix() const { return transform_matrix; }

    void set_metadata_matrix(const godot::Dictionary& p_matrix) { metadata_matrix = p_matrix; }
    godot::Dictionary get_metadata_matrix() const { return metadata_matrix; }

    void set_utility_matrix(const godot::Dictionary& p_matrix) { utility_matrix = p_matrix; }
    godot::Dictionary get_utility_matrix() const { return utility_matrix; }

    void set_manifest_version(int p_version) { manifest_version = p_version; }
    int get_manifest_version() const { return manifest_version; }
};

} // namespace ideam::core