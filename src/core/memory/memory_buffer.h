#ifndef IDEAM_CORE_MEMORY_BUFFER_H
#define IDEAM_CORE_MEMORY_BUFFER_H

#include "../memory/i_memory_buffer.h"
#include <vector>

namespace ideam::core {

/**
 * MemoryBuffer
 * * High-performance core object for memory data. 
 * Implements IMemoryBuffer to provide versioned, ID-based access.
 */
class MemoryBuffer : public IMemoryBuffer {
protected:
    uint32_t buffer_id = 0;
    uint32_t version = 0;

    uint8_t* master_block_ptr = nullptr;
    size_t memory_offset = 0;
    size_t capacity_bytes = 0;
    size_t used_bytes = 0;
    size_t alignment_requirement = 16;

    BufferAlignmentMode alignment_mode = BufferAlignmentMode::TIGHT;
    BufferLifecycleState lifecycle = BufferLifecycleState::IDLE;

public:
    MemoryBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode);
    virtual ~MemoryBuffer() override = default;

    // --- IMemoryBuffer Implementation ---
    [[nodiscard]] uint8_t* get_raw_ptr() const override { return master_block_ptr + memory_offset; }
    [[nodiscard]] size_t get_offset() const override { return memory_offset; }
    [[nodiscard]] size_t get_capacity() const override { return capacity_bytes; }
    [[nodiscard]] size_t get_used_size() const override { return used_bytes; }
    [[nodiscard]] size_t get_alignment() const override { return alignment_requirement; }
    
    // Assigned by MemoryManager. Used for pointer validation in Accessors and elsewhere.
    [[nodiscard]] uint32_t get_buffer_id() const override { return buffer_id; }

    [[nodiscard]] bool validate_rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) const override;
    void rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) override;

    void lock() override { lifecycle = BufferLifecycleState::LOCKED; }
    void unlock() override { lifecycle = BufferLifecycleState::IDLE; }
    [[nodiscard]] bool is_locked() const override { return lifecycle == BufferLifecycleState::LOCKED; }

    // --- MEMORY Pipeline Logic ---
    [[nodiscard]] uint32_t get_version() const override { return version; }

    [[nodiscard]] BufferAlignmentMode get_alignment_mode() const override { return alignment_mode; }
    [[nodiscard]] BufferLifecycleState get_lifecycle_state() const override { return lifecycle; }

    virtual void clear() {
        used_bytes = 0;
        version++;
    }

    /**
     * Incrementing the version forces any bound MemoryBufferAccessors 
     * to re-validate their internal pointers.
     */
    void bump_version() { version++; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_BUFFER_H