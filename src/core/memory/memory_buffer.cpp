#include "memory_buffer.h"

namespace ideam::core {

MemoryBuffer::MemoryBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode) 
    : buffer_id(p_buffer_id), alignment_mode(p_mode) {
}

bool MemoryBuffer::validate_rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) const {
    if (!p_new_master) {
        return false;
    }

    if (lifecycle == BufferLifecycleState::LOCKED) {
        return false;
    }

    // Alignment Check
    uintptr_t absolute_addr = reinterpret_cast<uintptr_t>(p_new_master) + p_new_offset;
    if (absolute_addr % alignment_requirement != 0) {
        return false;
    }

    // Capacity Check
    if (p_new_capacity < used_bytes) {
        return false;
    }

    return true;
}

void MemoryBuffer::rebase(uint8_t* p_new_master, size_t p_new_offset, size_t p_new_capacity) {
    // Note: It is assumed validate_rebase was called by the MemoryManager before this.
    master_block_ptr = p_new_master;
    memory_offset = p_new_offset;
    capacity_bytes = p_new_capacity;
    
    // Crucial for pointer invalidation in Accessors
    version++;
}

} // namespace ideam::core