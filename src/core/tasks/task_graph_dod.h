#ifndef IDEAM_CORE_TASK_GRAPH_DOD_H
#define IDEAM_CORE_TASK_GRAPH_DOD_H

#include "../memory/memory_graph_dod.h"
#include "../memory/memory_common.h"
#include "i_native_task.h"

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <vector>
#include <string>

namespace ideam::core {

/**
 * TaskTypeDOD
 * Determines which system owns the execution of this node.
 */
enum class TaskTypeDOD : uint8_t {
    GODOT_REFLECTION, 
    NATIVE_CPU,       
    COMPUTE_GPU,
    QUERY_CULLER      // New: Specialized type for logical selection filtering
};

/**
 * TaskPortMetadata
 * Maps a Graph Port to a specific DataType and alignment for Setup/Resolve.
 */
struct TaskPortMetadata {
    DataType type = DataType::FLOAT32;
    uint32_t buffer_id = INVALID_ID;
    uint32_t column_id = 0;
    uint32_t byte_size = 0;
};

/**
 * TaskPortConnectionDOD
 * Raw pointer-to-pointer link for high-speed inter-node data flow.
 */
struct TaskPortConnectionDOD {
    void* src_ptr = nullptr;
    void* dst_ptr = nullptr;
    size_t byte_size = 0;
};

/**
 * TaskCPUMetadata (Parallel Component)
 */
struct TaskCPUMetadata {
    std::string execution_method; 
    void* native_interface = nullptr; 
    godot::Object* reflection_target = nullptr;
};

/**
 * TaskGPUMetadata (Parallel Component)
 */
struct TaskGPUMetadata {
    godot::RID pipeline_rid;
    uint32_t dispatch_x = 1;
    uint32_t dispatch_y = 1;
    uint32_t dispatch_z = 1;
};

/**
 * TaskGraphDOD
 * Data-Oriented Task Execution Engine.
 */
class TaskGraphDOD : public MemoryGraphDOD {
protected:
    godot::RenderingDevice* rd = nullptr;
    
    // --- Component Storage (Parallel Arrays) ---
    std::vector<TaskTypeDOD> task_types;
    std::vector<TaskCPUMetadata> cpu_metadata;
    std::vector<TaskGPUMetadata> gpu_metadata;
    
    // Port mappings for automatic Memory Setup/Resolve
    std::vector<std::vector<TaskPortMetadata>> input_port_map;
    std::vector<std::vector<TaskPortMetadata>> output_port_map;

    // Fast-path connections (Bypasses Variants)
    std::vector<std::vector<TaskPortConnectionDOD>> baked_connections;

    // Cache for results/constants indexed by [NodeID][PortIndex]
    std::vector<std::vector<godot::Variant>> port_constants;

    // --- Internal Helpers ---
    void _bake_port_connections();
    
    // 3-Phase Execution Cycle
    void _batch_setup_wave(const NodeID* p_nodes, uint32_t p_count);
    void _batch_execute_wave(const NodeID* p_nodes, uint32_t p_count, double p_delta);
    void _batch_resolve_wave(const NodeID* p_nodes, uint32_t p_count);

    // Selection Maintenance
    void _clean_selections(NodeID p_id);

    // Marshalling
    void _variant_to_raw(const godot::Variant& p_var, void* p_dest, DataType p_type);
    godot::Variant _raw_to_variant(const void* p_src, DataType p_type);

    // --- Overrides ---
    virtual void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) override;

public:
    explicit TaskGraphDOD(MemoryManagerDOD* p_manager);
    virtual ~TaskGraphDOD() override = default;

    // --- Configuration ---
    NodeID add_task_node(TaskTypeDOD p_type);
    void configure_cpu_task(NodeID p_id, godot::Object* p_target, const std::string& p_method);
    void configure_gpu_task(NodeID p_id, godot::RID p_pipeline, uint32_t x, uint32_t y, uint32_t z);
    void configure_native_interface(NodeID p_id, INativeTask* p_interface);
    
    void set_port_mapping(NodeID p_id, bool p_input, uint32_t p_port_idx, DataType p_type, uint32_t p_buffer_id);
    void set_port_constant(NodeID p_id, uint32_t p_port_idx, const godot::Variant& p_value);

    /**
     * sync_with_manager
     * Refreshes internal pointers and RenderingDevice from the attached manager.
     */
    void sync_with_manager() {
        if (manager) {
            rd = manager->get_rendering_device();
            if (is_manager_version_dirty) {
                defragment(); 
                last_synced_version = *global_version_ptr;
            }
        }
    }

    // --- Execution ---
    void execute_graph_dod(double p_delta);

    virtual void defragment() override;
    void clear();

    // Grant access for execution logic
    [[nodiscard]] MemoryGrantPOD* get_grant_mutable(NodeID p_id);
};

} // namespace ideam::core

#endif // IDEAM_CORE_TASK_GRAPH_DOD_H