#ifndef IDEAM_CORE_SIMULATION_TASK_METADATA_H
#define IDEAM_CORE_SIMULATION_TASK_METADATA_H

#include "../tasks/task_graph.h"
#include "../memory/memory_buffer_selection.h"
#include <vector>
#include <cstdint>

namespace ideam::core {

/**
 * SimulationTaskMetadata
 * High-level configuration for simulation nodes.
 * Used by SimulationGraph to "bake" raw GrantParts for the MemoryManager.
 */
struct SimulationTaskMetadata {
    // Identity & Type (Utilizing the TaskType enum from TaskGraph)
    TaskType type = TaskType::NATIVE_CPU;
    

    // The list of buffers this simulation task needs to operate.
    std::vector<GrantPart> requirements;

    // --- Version Tracking ---
    // If the selection version changes, the SimulationGraph knows to 
    // flag the TaskGraph for a grant re-acquisition.
    uint32_t last_known_selection_version = 0;

    // Note: 'current_grant' has been removed. 
    // Grants are now managed by the persistent_grants map in MemoryGraph.
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATION_TASK_METADATA_H