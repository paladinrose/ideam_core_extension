#ifndef IDEAM_CORE_SIMULATIONS_COMMON_H
#define IDEAM_CORE_SIMULATIONS_COMMON_H

#include <cstdint>
#include <cstddef>

namespace ideam::core {
/**
 * BufferType
 * Identifies the memory layout and access logic for a specific prong.
 */
enum class SimulationType : int64_t {
    FIELD,
    MASS,
    ENTITY_COMPONENT
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATIONS_COMMON_H