#ifndef IDEAM_CORE_I_SIMULATION_BUFFER_H
#define IDEAM_CORE_I_SIMULATION_BUFFER_H

#include "simulations_common.h"
#include <cstdint>

namespace ideam::core {

class IMemoryBuffer;

/**
 * ISimulationBuffer
 * The non-template base used by SimulationManager to track logical simulation prongs.
 */
class ISimulationBuffer {
public:
    virtual ~ISimulationBuffer() = default;

    // --- Identification ---
    [[nodiscard]] virtual SimulationType get_simulation_type() const = 0;
    [[nodiscard]] virtual uint32_t get_buffer_id() const = 0;

    // --- Memory Linkage ---
    // Returns the internal memory implementation for registration with MemoryManager.
    [[nodiscard]] virtual IMemoryBuffer* get_memory_buffer() = 0;
    [[nodiscard]] virtual const IMemoryBuffer* get_memory_buffer() const = 0;

    // --- Lifecycle Hooks ---
    virtual void on_simulation_start() = 0;
    virtual void on_simulation_stop() = 0;

    // --- Versioning ---
    // Proxies the internal MemoryBuffer version for Accessor validation.
    [[nodiscard]] virtual uint32_t get_version() const = 0;
    virtual void bump_version() = 0;
};

} // namespace ideam::core

#endif // IDEAM_CORE_I_SIMULATION_BUFFER_H