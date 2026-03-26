#ifndef IDEAM_CORE_SIMULATION_MANAGER_H
#define IDEAM_CORE_SIMULATION_MANAGER_H

#include "../memory/memory_manager.h"
#include "../memory/memory_common.h"
#include "../memory/i_memory_buffer.h"
#include "simulation_graph.h"
#include "i_simulation_buffer.h"
#include "simulation_task_metadata.h"
#include <vector>
#include <memory>

namespace ideam::core {

class SimulationGraph;

/**
 * ExecutionTiming
 * Mirrors Godot's execution methodology. 
 * The Simulator Node will map its callbacks to these buckets.
 */
enum class ExecutionTiming : int64_t {
    MANUAL = 0,
    PROCESS = 1,
    PHYSICS_PROCESS = 2,
    FIXED_INTERVAL = 3
};

/**
 * SimulationManager
 * The high-performance orchestrator for the Ideam simulation engine.
 * * DESIGN NOTES:
 * 1. This class is now pure C++ (ideam::core) and agnostic of Godot types.
 * 2. It owns the MemoryManager and handles the "When" of the simulation.
 * 3. It provides the execution buckets that a Godot-facing 'Simulator' Node will pump.
 */
class SimulationManager {
private:
    // The central authority for memory and security grants.
    std::unique_ptr<MemoryManager> memory_manager;

    // --- Execution Buckets ---
    struct GraphEntry { 
        SimulationGraph* graph; 
        double timer = 0.0; 
        double interval = 0.1; 
    };

    std::vector<SimulationGraph*> graph_bucket_process;
    std::vector<SimulationGraph*> graph_bucket_physics;
    std::vector<SimulationGraph*> graph_bucket_manual;
    std::vector<GraphEntry> graph_bucket_interval;

public:
    SimulationManager();
    ~SimulationManager();

    // --- Memory & Buffer Lifecycle ---
    // Delegates directly to the internal MemoryManager.
    void register_buffer(ISimulationBuffer* p_buffer);
    void unregister_buffer(ISimulationBuffer* p_buffer);

    /**
     * request_grant
     * Acquires a security token from the MemoryManager based on Task requirements.
     */
    MemoryGrant* request_grant(SimulationTaskMetadata* p_task);
    
    /**
     * release_grant
     * Returns a security token to the MemoryManager pool.
     */
    void release_grant(MemoryGrant* p_grant);

    // --- Graph Orchestration ---
    /**
     * register_managed_graph
     * Places a graph into an execution bucket.
     */
    void register_managed_graph(SimulationGraph* p_graph, ExecutionTiming p_timing, double p_interval = 0.1);
    void unregister_managed_graph(SimulationGraph* p_graph);
    
    // --- Execution Entry Points ---
    // These are intended to be called by a Godot Node or a high-level Simulator.
    void execute_process(double p_delta);
    void execute_physics(double p_delta);
    void execute_manual(double p_delta);
    void execute_interval(double p_delta);

    // --- Metadata Access ---
    [[nodiscard]] MemoryManager* get_memory_manager() const { return memory_manager.get(); }
    [[nodiscard]] uint32_t get_version() const { return memory_manager->get_version(); }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATION_MANAGER_H