#ifndef IDEAM_CORE_MEMORY_I_MEMORY_BUFFER_H
#define IDEAM_CORE_MEMORY_I_MEMORY_BUFFER_H

#include "memory_common.h"
#include <cstdint>
#include <cstddef>

namespace ideam::core {

/**
 * IMemoryBuffer
 * A neutral interface for any component requiring managed, aligned contiguous memory.
 */
class IMemoryBuffer {
public:
    virtual ~IMemoryBuffer() = default;

    // --- Access ---
    [[nodiscard]] virtual uint8_t* get_raw_ptr() const = 0;
    [[nodiscard]] virtual size_t get_offset() const = 0;
    [[nodiscard]] virtual size_t get_capacity() const = 0;
    [[nodiscard]] virtual size_t get_used_size() const = 0;
    [[nodiscard]] virtual size_t get_alignment() const = 0;
    [[nodiscard]] virtual uint32_t get_buffer_id() const = 0;
    [[nodiscard]] virtual uint32_t get_version() const = 0;
    [[nodiscard]] virtual BufferAlignmentMode get_alignment_mode() const = 0;
    [[nodiscard]] virtual BufferLifecycleState get_lifecycle_state() const = 0;
    
    // --- Management ---
    virtual void rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) = 0;
    
    /**
     * validate_rebase
     * Checks if the proposed memory slice satisfies the buffer's alignment 
     * and size requirements before the actual move occurs.
     */
    [[nodiscard]] virtual bool validate_rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) const = 0;

    // --- Lifecycle ---
    virtual void lock() = 0;
    virtual void unlock() = 0;
    [[nodiscard]] virtual bool is_locked() const = 0;
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_I_MEMORY_BUFFER_H