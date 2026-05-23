#pragma once

#include "task_resource.h"
#include <godot_cpp/classes/resource.hpp>

namespace ideam::godot_ext {

// Forward declaration to prevent cyclic inclusion during Graph Compiler builds
class TaskGraphResource;

/**
 * @class SubGraphTaskResource
 * @brief Authoring payload for nested graph execution.
 * * Separates the heavy associative mappings (Dictionaries) and strong resource references 
 * used in the Editor from the strict, contiguous `GrantMapping` vectors used by the 
 * runtime `SubGraphTask`. The compiler will bake these editor constraints down to raw IDs.
 */
class SubGraphTaskResource : public TaskResource {
    GDCLASS(SubGraphTaskResource, TaskResource)

private:
    // Strong reference to the child graph payload. The Graph Compiler will recursively
    // unpack this to instantiate the native TaskGraphDOD.
    godot::Ref<TaskGraphResource> child_graph;

    // Authoring associative map: Parent Buffer ID (Key) -> Child Node ID (Value).
    // Avoids dynamic allocations at runtime by acting strictly as a compilation template.
    godot::Dictionary grant_mappings;

protected:
    static void _bind_methods();

public:
    SubGraphTaskResource();
    ~SubGraphTaskResource() override;

    // --- Graph Aliasing Configuration ---
    void set_child_graph(const godot::Ref<TaskGraphResource>& p_graph);
    godot::Ref<TaskGraphResource> get_child_graph() const;

    void set_grant_mappings(const godot::Dictionary& p_mappings);
    godot::Dictionary get_grant_mappings() const;
};

} // namespace ideam::godot_ext