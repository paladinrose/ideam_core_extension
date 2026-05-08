#pragma once

#include "../memory/memory_graph_node_resource.h"

namespace ideam::godot_ext {

/**
 * @class TaskGraphNodeResource
 * @brief The baseline Authoring component for executable graph instructions.
 * * Extends MemoryGraphNodeResource to define scheduling and threading constraints.
 * Specific execution logic (Transform, Query, Metadata) is deferred to derived classes.
 * The Graph Compiler uses this layer to build thread-safe execution queues 
 * (e.g., job system dependency barriers) before fetching the actual task payloads.
 */
class TaskGraphNodeResource : public MemoryGraphNodeResource {
    GDCLASS(TaskGraphNodeResource, MemoryGraphNodeResource)

public:
    // Defines how the runtime DOD task scheduler should dispatch this node.
    // Grouping tasks by parallel vs. serial execution allows the graph to 
    // maximize thread pool saturation without causing race conditions.
    enum ExecutionMode {
        EXEC_SERIAL = 0,    // Must run sequentially (e.g., mutates global state)
        EXEC_PARALLEL = 1   // Thread-safe / SoA compatible (e.g., mapping over a contiguous array)
    };

private:
    ExecutionMode exec_mode = EXEC_PARALLEL;
    
    // Allows designers to safely bypass nodes in the Editor without destroying
    // their topological edge connections. The compiler will simply prune this 
    // node from the final DOD execution array if false.
    bool is_active = true;

protected:
    static void _bind_methods();

public:
    TaskGraphNodeResource() = default;
    ~TaskGraphNodeResource() override = default;

    // --- Scheduling Configuration ---
    void set_execution_mode(int p_mode);
    int get_execution_mode() const;

    void set_is_active(bool p_active);
    bool get_is_active() const;

    // --- DOD Compilation Interface ---
    
    /**
     * @brief Validates the task's scheduling constraints, cascading up to 
     * ensure memory capacities and topological identities are also sound.
     */
    bool validate_for_compilation() const override;
};

} // namespace ideam::godot_ext

// Expose enum to Godot globally
VARIANT_ENUM_CAST(ideam::godot_ext::TaskGraphNodeResource::ExecutionMode);