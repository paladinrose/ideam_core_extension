#ifndef IDEAM_CORE_SIMULATION_SYSTEM_H
#define IDEAM_CORE_SIMULATION_SYSTEM_H

#include "simulation_task_metadata.h"
#include "../memory/memory_buffer_accessor.h"
#include <vector>

namespace ideam::core {

/**
 * SimulationSystem
 * * Replaces the old ISimulationTask, SimulationSystem, and BridgeSystem.
 * A System is now defined by its Accessor requirements.
 * * DESIGN:
 * 1. Stateless: Systems should ideally hold no per-instance data.
 * 2. Signature-Driven: The system tells the Graph what it needs via Metadata.
 * 3. Unified: A 'Bridge' is simply a system with requirements.size() > 1.
 */
class SimulationSystem {
public:
    virtual ~SimulationSystem() = default;

    /**
     * get_system_metadata
     * Returns the template for this system's requirements.
     * The SimulationGraph uses this during the 'Bake' phase to:
     * - Request appropriate Grants.
     * - Initialize the correct number/type of Accessors.
     */
    [[nodiscard]] virtual SimulationTaskMetadata get_system_metadata() const = 0;

    /**
     * execute
     * The hot-loop execution entry point.
     * * @param p_accessors: A vector of pointers to pre-resolved accessors.
     * The order of accessors matches the order of 'requirements' in metadata.
     */
    virtual void execute(std::vector<MemoryBufferAccessor<uint8_t>*> p_accessors) = 0;

    /**
     * get_system_id
     * Used for registry lookups within the SimulationManager.
     */
    [[nodiscard]] virtual uint32_t get_system_id() const = 0;
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATION_SYSTEM_H