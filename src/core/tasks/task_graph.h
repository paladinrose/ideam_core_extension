#ifndef IDEAM_CORE_TASK_GRAPH_H
#define IDEAM_CORE_TASK_GRAPH_H

#include "../memory/memory_graph.h"
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <string>
#include <vector>

namespace ideam::core {

enum class TaskType : uint8_t {
    GODOT_REFLECTION, 
    NATIVE_CPU,       
    COMPUTE_GPU       
};

/**
 * INativeTask
 * Direct C++ interface. Receives the MemoryGrant (Lease) for high-performance access.
 */
class INativeTask {
public:
    virtual ~INativeTask() = default;
    virtual void execute_native(MemoryGrant* p_grant) = 0;
};

struct TaskIOMapping {
    uint32_t port_id = 0;         
    std::string target_name;      
    bool is_return_value = false; 
};

struct TaskMetadata {
    TaskType type = TaskType::NATIVE_CPU;
    
    // CPU Logic
    std::string execution_method;
    INativeTask* native_interface = nullptr;

    // GPU Logic
    godot::RID shader_rid;
    godot::RID pipeline_rid;
    uint32_t dispatch_x = 1;
    uint32_t dispatch_y = 1;
    uint32_t dispatch_z = 1;

    // Port Mappings
    std::vector<TaskIOMapping> input_mappings;
    std::vector<TaskIOMapping> output_mappings;

    godot::Dictionary input_values;
    godot::Dictionary output_values;
    godot::Variant last_return_value; 
    
    bool has_executed = false;
};

class TaskGraph : public MemoryGraph {
private:
    std::vector<TaskMetadata> task_registry;
    godot::RenderingDevice* rd = nullptr;

    // Internal execution helpers
    void _execute_node_logic(NodeID p_id, godot::Object* p_executor = nullptr);
    void _dispatch_gpu_task(NodeID p_id, int64_t p_compute_list);

public:
    TaskGraph();
    virtual ~TaskGraph() override;

    // --- Node API ---
    NodeID add_task_node(TaskType p_type = TaskType::NATIVE_CPU);
    void remove_node(NodeID p_id);
    void defragment();
    void clear();

    void validate_grants() override;
    // --- Execution Engine (Stage D) ---
    /**
     * execute_graph
     * Iterates through Kahn waves, validates memory leases, and dispatches
     * logic across CPU and GPU.
     */
    void execute_graph(double p_delta);

    // --- Configuration ---
    TaskMetadata* get_task(NodeID p_id);
    void set_task_input(NodeID p_id, uint32_t p_port_id, const godot::Variant& p_value);
    godot::Variant get_task_output(NodeID p_id, uint32_t p_port_id);

    // --- Reflection Phases ---
    void task_phase_setup(NodeID p_id, godot::Object* p_executor);
    void task_phase_execute(NodeID p_id, godot::Object* p_executor);
    void task_phase_resolve(NodeID p_id, godot::Object* p_executor);

    virtual void on_topology_changed() override;
};

} // namespace ideam::core

#endif // IDEAM_CORE_TASK_GRAPH_H