#pragma once

#include "../memory/memory_graph_node_resource.h"

namespace ideam::godot_ext {

/**
 * TaskType
 * Godot-exposed mirror of core::TaskTypeDOD. Moved here so individual nodes
 * can explicitly type themselves without dynamic dictionary parsing.
 */
enum TaskType : uint32_t {
    TASK_GODOT_REFLECTION = 0,
    TASK_NATIVE_CPU       = 1,
    TASK_COMPUTE_GPU      = 2,
    TASK_QUERY_CULLER     = 3
};

/**
 * @class TaskResource
 * @brief The baseline Authoring component for executable graph instructions.
 * * Extends MemoryGraphNodeResource to define scheduling and threading constraints.
 * Specific execution logic (Transform, Query, Metadata) is deferred to derived classes.
 * The Graph Compiler uses this layer to build thread-safe execution queues 
 * (e.g., job system dependency barriers) before fetching the actual task payloads.
 */
class TaskResource : public MemoryGraphNodeResource {
    GDCLASS(TaskResource, MemoryGraphNodeResource)

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

    // Abstracted away from loosely-packed dictionaries to guarantee fast 
    // deterministic memory access during compiler instantiation.
    TaskType task_type = TASK_NATIVE_CPU;
    
    // Fallback associative map reserved purely for dynamic Native Component initialization.
    godot::Dictionary task_properties; 
    godot::StringName task_name;
   
protected:

    uint32_t logic_id = 0;
    
    // The primary/root ID for this family of logics (e.g., the ID of LOD_1Level)
    uint32_t base_logic_id = 0; 
    
    // Cached UI strings for the dropdown
    godot::PackedStringArray logic_variant_labels;

    
    static void _bind_methods();

public:
    TaskResource() = default;
    ~TaskResource() override = default;

    // --- Scheduling Configuration ---
    void set_execution_mode(int p_mode);
    int get_execution_mode() const;

    void set_is_active(bool p_active);
    bool get_is_active() const;

    void set_task_type(int p_type);
    int get_task_type() const;

    void set_task_name(const godot::StringName& p_name);
    godot::StringName get_task_name() const;

    void set_task_properties(const godot::Dictionary& p_props);
    virtual godot::Dictionary get_task_properties() const;

    void set_logic_id(uint32_t p_id);
    uint32_t get_logic_id() const;

    void set_base_logic_id(uint32_t p_id);
    uint32_t get_base_logic_id() const;
    
    void set_logic_variant_labels(const godot::PackedStringArray& p_labels);
    godot::PackedStringArray get_logic_variant_labels() const;
    // --- DOD Compilation Interface ---
    
    /**
     * @brief Validates the task's scheduling constraints, cascading up to 
     * ensure memory capacities and topological identities are also sound.
     */
    bool validate_for_compilation() const override;
};

} // namespace ideam::godot_ext

// Expose enums to Godot globally
VARIANT_ENUM_CAST(ideam::godot_ext::TaskType);
VARIANT_ENUM_CAST(ideam::godot_ext::TaskResource::ExecutionMode);