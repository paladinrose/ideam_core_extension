#include "simulation_graph_dod.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::core {

SimulationGraphDOD::SimulationGraphDOD(MemoryManagerDOD* p_memory_manager) 
    : TaskGraphDOD(p_memory_manager) {
    // TaskGraphDOD handles the initial sync with the manager.
}

NodeID SimulationGraphDOD::add_compute_system(const godot::RID& p_pipeline_rid, uint32_t x, uint32_t y, uint32_t z) {
    NodeID id = add_task_node(TaskTypeDOD::COMPUTE_GPU);
    configure_gpu_task(id, p_pipeline_rid, x, y, z);
    has_gpu_tasks = true;
    return id;
}

void SimulationGraphDOD::execute_frame(double p_delta) {
    if (!manager) {
        return;
    }

    // 1. Memory Audit
    // Ensures all MemoryGrantPODs are updated if the MemoryManager performed a rebase.
    validate_grants();

    // 2. Topology/Resource Re-bake
    // If ports were remapped or connections changed, we rebuild the fast-path memcpy links.
    if (dirty_flags & RESOURCES) {
        _bake_port_connections();
        dirty_flags &= ~RESOURCES;
    }

    // 3. Dispatch Wave-based Execution
    // This calls TaskGraphDOD::execute_graph_dod which handles:
    // - _batch_setup_wave (Memcpy inter-node data)
    // - _batch_execute_wave (Native C++ / GPU Compute / Godot Reflection)
    // - _batch_resolve_wave (Variant conversion for Godot UI/Scripts)
    execute_graph_dod(p_delta);
}

void SimulationGraphDOD::defragment() {
    // 1. Perform base remapping of Node/Edge IDs
    TaskGraphDOD::defragment();

    // 2. The _remap_ids override (below) handles the movement of 
    // unique_ptr systems and parallel metadata arrays.
}

void SimulationGraphDOD::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    // Call parent to remap TaskGraphDOD's internal component arrays (task_types, port_maps, etc.)
    TaskGraphDOD::_remap_ids(p_node_lut, p_edge_lut);

    // Create a new container for the reordered systems
    std::unordered_map<NodeID, std::unique_ptr<INativeTask>> remapped_systems;

    for (uint32_t i = 0; i < p_node_lut.size(); ++i) {
        NodeID new_id = p_node_lut[i];
        
        if (new_id != INVALID_ID) {
            auto it = native_systems.find(i);
            if (it != native_systems.end()) {
                // Move ownership to the new ID location
                remapped_systems[new_id] = std::move(it->second);
                
                // Update the TaskGraphDOD's native_interface pointer to point to the new location
                if (new_id < cpu_metadata.size()) {
                    cpu_metadata[new_id].native_interface = remapped_systems[new_id].get();
                }
            }
        }
    }

    native_systems = std::move(remapped_systems);
    
    // Scan for GPU tasks to update the has_gpu_tasks hint
    has_gpu_tasks = false;
    for (const auto& type : task_types) {
        if (type == TaskTypeDOD::COMPUTE_GPU) {
            has_gpu_tasks = true;
            break;
        }
    }
}

void SimulationGraphDOD::clear() {
    native_systems.clear();
    has_gpu_tasks = false;
    TaskGraphDOD::clear();
}

} // namespace ideam::core