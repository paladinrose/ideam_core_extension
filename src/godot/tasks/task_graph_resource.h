#ifndef IDEAM_GODOT_TASK_GRAPH_RESOURCE_H
#define IDEAM_GODOT_TASK_GRAPH_RESOURCE_H

#include "../memory/memory_graph_resource.h"
#include "../../core/tasks/task_graph_dod.h"
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

/**
 * TaskType
 * Godot-exposed mirror of core::TaskTypeDOD
 */
enum TaskType : uint32_t {
    TASK_GODOT_REFLECTION = 0,
    TASK_NATIVE_CPU       = 1,
    TASK_COMPUTE_GPU      = 2,
    TASK_QUERY_CULLER     = 3
};

class TaskGraphResource : public MemoryGraphResource {
    GDCLASS(TaskGraphResource, MemoryGraphResource)

protected:
    static void _bind_methods();

public:
    TaskGraphResource() = default;
    virtual ~TaskGraphResource() override = default;

    /**
     * @brief Compiles the Godot topological blueprint into an executable TaskGraphDOD.
     * Routes node configurations based on their serialized task_type and properties.
     */
    std::shared_ptr<core::TaskGraphDOD> compile_to_task_graph(
        core::MemoryManagerDOD* p_manager, 
        godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TaskType);

#endif // IDEAM_GODOT_TASK_GRAPH_RESOURCE_H