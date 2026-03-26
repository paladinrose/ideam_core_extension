#ifndef IDEAM_CORE_SIMULATIONS_SIMULATION_GRAPH_H
#define IDEAM_CORE_SIMULATIONS_SIMULATION_GRAPH_H

#include "../tasks/task_graph.h"
#include "simulation_task_metadata.h"
#include <unordered_map>

namespace ideam::core {

class SimulationManager;

/**
 * SimulationGraph
 * Extends TaskGraph to provide high-level simulation logic management.
 * Connects domain-specific SimulationTaskMetadata to the raw TaskGraph/MemoryManager pipeline.
 */
class SimulationGraph : public TaskGraph {
private:
    SimulationManager* manager = nullptr;
    
    // Registry for simulation-specific requirements (Selections, Buffer IDs, etc.)
    std::unordered_map<NodeID, SimulationTaskMetadata> sim_metadata;

public:
    SimulationGraph() = default;
    virtual ~SimulationGraph() override = default;

    void set_manager(SimulationManager* p_mgr) { manager = p_mgr; }

    /**
     * add_simulation_task
     * Registers a node in the TaskGraph and stores its simulation-specific requirements.
     */
    NodeID add_simulation_task(TaskType p_type, const std::vector<SimulationTaskMetadata::BufferAccess>& p_reqs);

    /**
     * execute_frame
     * The simulation entry point. Now acts as a thin wrapper around TaskGraph::execute_graph
     * to ensure the new wave-based execution and GPU synchronization logic is used.
     */
    void execute_frame(double p_delta);

    /**
     * _bake_requirements (Override)
     * Translates SimulationTaskMetadata::BufferAccess into TaskGraph/MemoryGraph GrantParts.
     * This is called automatically by validate_grants() when the topology or resources change.
     */
    virtual void _bake_requirements() override;
    
    SimulationTaskMetadata* get_sim_metadata(NodeID p_id);
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATIONS_SIMULATION_GRAPH_H