#pragma once

#include "../memory/memory_graph_dod.h"
#include "../memory/memory_common.h"
#include "i_native_task.h"

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <vector>
#include <memory>
#include <span>

namespace ideam::core {

enum class TaskTypeDOD : uint8_t {
    GODOT_REFLECTION, 
    NATIVE_CPU,       
    COMPUTE_GPU,
    QUERY_CULLER
};

struct TaskPortMetadata {
    DataType type = DataType::FLOAT32;
    uint32_t buffer_id = INVALID_ID;
    uint32_t column_id = 0;
    uint32_t byte_size = 0;
};

struct TaskPortConnectionDOD {
    void* src_ptr = nullptr;
    void* dst_ptr = nullptr;
    uint32_t byte_size = 0;
};

struct TaskCPUMetadata {
    godot::Object* reflection_target = nullptr;
    godot::StringName execution_method; 
    INativeTask* native_interface = nullptr;
};

struct TaskGPUMetadata {
    godot::RID pipeline_rid;
    uint32_t dispatch_x = 1;
    uint32_t dispatch_y = 1;
    uint32_t dispatch_z = 1;
};

struct TaskPortOffsets {
    uint32_t offset = 0;
    uint32_t count = 0;
};

class TaskGraphDOD : public MemoryGraphDOD {
protected:
    std::vector<TaskTypeDOD> task_types;
    std::vector<TaskCPUMetadata> cpu_metadata;
    std::vector<TaskGPUMetadata> gpu_metadata;

    // --- Lifecycle Management ---
    std::vector<std::unique_ptr<INativeTask>> owned_native_tasks;

    // --- CSR Flattened Port Mappings ---
    std::vector<TaskPortOffsets> input_port_meta;
    std::vector<TaskPortMetadata> input_port_data;

    std::vector<TaskPortOffsets> output_port_meta;
    std::vector<TaskPortMetadata> output_port_data;

    std::vector<TaskPortOffsets> constant_port_meta;
    std::vector<godot::Variant> constant_port_data;

    std::vector<std::vector<TaskPortConnectionDOD>> baked_connections;

    // --- Transient Memory ---
    std::vector<uint32_t> transient_bytes_meta;
    
    virtual size_t _get_node_transient_requirement(NodeID p_id) const override {
        if (p_id < transient_bytes_meta.size()) return transient_bytes_meta[p_id];
        return 0;
    }

    godot::RenderingDevice* rd = nullptr;

    // --- Command Buffer Resources ---
    uint32_t tier1_buffer_id = INVALID_ID;
    uint32_t tier2_buffer_id = INVALID_ID;
    
    std::vector<TaskGraphCommandPOD> tier1_meta;
    std::vector<TaskSelectionCommandPOD> tier2_meta;

    std::vector<std::shared_ptr<TaskGraphDOD>> child_graphs;

    // --- Internal Execution Pipeline ---
    void _bake_port_connections();
    void _clean_selections(NodeID p_id, void* p_workspace);
    
    void _batch_prepare_wave(const NodeID* p_nodes, uint32_t p_count, double p_delta, void** p_workspaces);
    void _batch_setup_wave(const NodeID* p_nodes, uint32_t p_count);
    void _batch_execute_wave(const NodeID* p_nodes, uint32_t p_count, double p_delta, void** p_workspaces);
    void _batch_resolve_wave(const NodeID* p_nodes, uint32_t p_count);
    
    void _process_tier2_commands(const NodeID* p_nodes, uint32_t p_count);
    void _process_tier1_commands();

    // --- Data Converters ---
    void _variant_to_raw(const godot::Variant& p_var, void* p_dest, DataType p_type);
    godot::Variant _raw_to_variant(const void* p_src, DataType p_type);

    // --- Overrides ---
    virtual void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) override;

public:
    explicit TaskGraphDOD(MemoryManagerDOD* p_manager);
    virtual ~TaskGraphDOD() override = default;

    inline void retain_child_graph(std::shared_ptr<TaskGraphDOD> p_child) {
        child_graphs.push_back(p_child);
    }
    
    // --- Configuration ---
    NodeID add_task_node(TaskTypeDOD p_type);
    
    void configure_command_arenas(size_t p_tier1_bytes, size_t p_tier2_bytes);
    
    void configure_cpu_task(NodeID p_id, godot::Object* p_target, const godot::StringName& p_method);
    void configure_gpu_task(NodeID p_id, godot::RID p_pipeline, uint32_t x, uint32_t y, uint32_t z);
    
    // Updated: Now takes a unique_ptr to assume ownership.
    void configure_native_interface(NodeID p_id, std::unique_ptr<INativeTask> p_interface);
    
    void set_node_transient_requirement(NodeID p_id, uint32_t p_bytes);
    
    void set_port_mappings(NodeID p_id, bool p_input, std::span<const TaskPortMetadata> p_mappings);
    void set_port_constants(NodeID p_id, std::span<const godot::Variant> p_constants);

    MemoryGrantPOD* get_grant_mutable(NodeID p_id);

    [[nodiscard]] inline std::span<const TaskGraphCommandPOD> get_tier1_commands() const noexcept {
        return tier1_meta;
    }
    
    void sync_with_manager() {
        if (manager) {
            rd = manager->get_rendering_device();
            if (is_manager_version_dirty()) {
                defragment(); 
                last_synced_version = *global_version_ptr;
            }
        }
    }
    
    void execute_graph_dod(double p_delta);
    
    virtual void defragment() override;
    void clear();
};

} // namespace ideam::core

 // IDEAM_CORE_TASK_GRAPH_DOD_H