#ifndef IDEAM_CORE_SIMULATION_GRAPH_DOD_H
#define IDEAM_CORE_SIMULATION_GRAPH_DOD_H

#include "../tasks/task_graph_dod.h"
#include "simulation_manager_dod.h"
#include <unordered_map>
#include <string>

namespace ideam::core {

/**
 * SimulationGraphDOD
 * The high-level container for simulation "Systems".
 * * DESIGN NOTES:
 * 1. Inherits TaskGraphDOD for wave-based, multi-threaded execution.
 * 2. Manages the lifecycle of INativeTask (System) instances.
 * 3. Provides the "Lego" interface for Godot-facing wrappers.
 */
class SimulationGraphDOD : public TaskGraphDOD {
private:
    // Non-owning pointer back to the orchestrator for global context.
    SimulationManagerDOD* manager = nullptr;

    // --- System Ownership ---
    // Maps NodeID to the specific C++ implementation of a simulation logic.
    std::unordered_map<NodeID, std::unique_ptr<INativeTask>> native_systems;

    // Performance Hint: Tracking if we have GPU tasks to avoid unnecessary RD syncs.
    bool has_gpu_tasks = false;

protected:
    // --- Overrides ---
    virtual void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) override;

public:
    explicit SimulationGraphDOD(MemoryManagerDOD* p_memory_manager);
    virtual ~SimulationGraphDOD() override = default;

    // --- Manager Integration ---
    void set_manager(SimulationManagerDOD* p_manager) { manager = p_manager; }
    [[nodiscard]] SimulationManagerDOD* get_manager() const { return manager; }

    // --- System Management ---
    /**
     * add_system
     * Registers a concrete C++ INativeTask as a node in the graph.
     * T is any class implementing INativeTask.
     */
    template <typename T, typename... Args>
    NodeID add_system(Args&&... p_args) {
        NodeID id = add_task_node(TaskTypeDOD::NATIVE_CPU);
        
        auto system = std::make_unique<T>(std::forward<Args>(p_args)...);
        cpu_metadata[id].native_interface = system.get();
        native_systems[id] = std::move(system);
        
        return id;
    }

    /**
     * add_compute_system
     * Registers a GPU-based task (HLSL/GLSL compute shader).
     */
    NodeID add_compute_system(const godot::RID& p_pipeline_rid, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1);

    // --- Execution Pipeline ---
    /**
     * execute_frame
     * The primary entry point pumped by the SimulationManagerDOD.
     * This handles the full Setup -> Execute -> Resolve cycle.
     */
    void execute_frame(double p_delta);

    // --- State Management ---
    virtual void defragment() override;
    void clear();

    /**
     * get_system_by_id
     * Returns a pointer to the specific system for configuration (e.g. Gravity settings).
     */
    template <typename T>
    T* get_system(NodeID p_id) {
        auto it = native_systems.find(p_id);
        if (it != native_systems.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATION_GRAPH_DOD_H