#ifndef IDEAM_CORE_MEMORY_RAW_BUFFER_H
#define IDEAM_CORE_MEMORY_RAW_BUFFER_H

#include "i_memory_buffer.h"

namespace ideam::core {

/**
 * RawBuffer
 * A concrete, simulation-agnostic implementation of IMemoryBuffer.
 * Handles the mechanical aspects of master-block slicing and alignment.
 */
class RawBuffer : public IMemoryBuffer {
protected:
    uint8_t* master_block_ptr = nullptr;
    size_t offset = 0;
    size_t capacity_bytes = 0;
    size_t used_bytes = 0;
    size_t alignment_requirement = 16;
    
    bool locked = false;

public:
    RawBuffer(size_t p_alignment = 16) : alignment_requirement(p_alignment) {}
    virtual ~RawBuffer() override = default;

    // --- IMemoryBuffer Implementation ---

    [[nodiscard]] inline uint8_t* get_raw_ptr() const override {
        return master_block_ptr + offset;
    }

    [[nodiscard]] inline size_t get_offset() const override { return offset; }
    [[nodiscard]] inline size_t get_capacity() const override { return capacity_bytes; }
    [[nodiscard]] inline size_t get_used_size() const override { return used_bytes; }
    [[nodiscard]] inline size_t get_alignment() const override { return alignment_requirement; }

    virtual void rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) override {
        master_block_ptr = p_new_master;
        offset = p_new_offset;
        capacity_bytes = p_new_capacity;
    }

    [[nodiscard]] virtual bool validate_rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) const override {
        if (!p_new_master || locked) {
            return false;
        }

        // Check alignment of the resulting absolute pointer
        const uintptr_t absolute_addr = reinterpret_cast<uintptr_t>(p_new_master) + p_new_offset;
        if (absolute_addr % alignment_requirement != 0) {
            return false;
        }

        // Ensure we aren't shrinking below currently used data
        if (p_new_capacity < used_bytes) {
            return false;
        }

        return true;
    }

    void lock() override { locked = true; }
    void unlock() override { locked = false; }
    [[nodiscard]] bool is_locked() const override { return locked; }

    // --- Neutral Utilities ---
    void set_used_size(size_t p_size) { used_bytes = p_size; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_RAW_BUFFER_H