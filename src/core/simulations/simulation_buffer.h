#ifndef IDEAM_CORE_SIMULATION_BUFFER_H
#define IDEAM_CORE_SIMULATION_BUFFER_H

#include "i_simulation_buffer.h"
#include "../memory/memory_common.h"
#include <memory>
#include <type_traits>

namespace ideam::core {

/**
 * SimulationBuffer<TMemoryBuffer>
 * * Wraps a specific MemoryBuffer implementation (SoA, AoS, SparseSet).
 * * Owns the memory implementation rather than inheriting from it.
 * * TMemoryBuffer must implement the IMemoryBuffer interface.
 */
template <typename TMemoryBuffer>
class SimulationBuffer : public ISimulationBuffer {
    static_assert(std::is_base_of_v<IMemoryBuffer, TMemoryBuffer>, 
        "TMemoryBuffer must derive from IMemoryBuffer");

private:
    // The underlying memory storage (e.g., StructureOfArraysBuffer)
    std::unique_ptr<TMemoryBuffer> internal_buffer;
    
    SimulationType sim_type;
    uint32_t buffer_id;

public:
    SimulationBuffer(uint32_t p_id, SimulationType p_sim_type, BufferAlignmentMode p_align_mode)
        : buffer_id(p_id), sim_type(p_sim_type) {
        
        // Initialize the specific memory buffer implementation
        internal_buffer = std::make_unique<TMemoryBuffer>(p_id, p_align_mode);
    }

    virtual ~SimulationBuffer() override = default;

    // --- ISimulationBuffer Implementation ---

    [[nodiscard]] SimulationType get_simulation_type() const override { 
        return sim_type; 
    }

    [[nodiscard]] uint32_t get_buffer_id() const override { 
        return buffer_id; 
    }

    [[nodiscard]] IMemoryBuffer* get_memory_buffer() override { 
        return internal_buffer.get(); 
    }

    [[nodiscard]] const IMemoryBuffer* get_memory_buffer() const override { 
        return internal_buffer.get(); 
    }

    [[nodiscard]] uint32_t get_version() const override { 
        return internal_buffer->get_version(); 
    }

    void bump_version() override { 
        internal_buffer->bump_version(); 
    }

    void on_simulation_start() override {
        // Logic for prepping simulation (e.g., locking structure)
        internal_buffer->lock();
    }

    void on_simulation_stop() override {
        // Logic for releasing simulation (e.g., unlocking for resizing)
        internal_buffer->unlock();
    }

    // --- Typed Access ---
    /**
     * get_internal
     * Provides access to the underlying specialized buffer (e.g., for resizing).
     */
    [[nodiscard]] TMemoryBuffer* get_internal() { 
        return internal_buffer.get(); 
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATION_BUFFER_H