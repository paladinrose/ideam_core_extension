#pragma once

#include <cstdint>
#include <cassert>

namespace ideam::core {

/**
 * TaskSelectionCommandPOD
 * A lightweight, thread-local bump buffer for deferring selection additions.
 * Locked to exactly 32 bytes for perfect half-cache-line packing.
 */
struct TaskSelectionCommandPOD {
    // --- 8-Byte Alignment Block (24 bytes) ---
    int64_t* queued_indices = nullptr; // Pointer to a pre-allocated graph memory arena
    int64_t count = 0;                 // Number of indices queued in this wave
    int64_t capacity = 0;              // Maximum allowed bounds of the arena chunk

    // --- 4-Byte Alignment Block (8 bytes) ---
    uint32_t target_buffer_id = 0;     // The buffer selection this command will modify
    
    // Explicit padding to lock the struct at 32 bytes
    uint32_t reserved_padding = 0;

    /**
     * push_addition
     * High-speed, branch-predictable inline push for tasks expanding a selection.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void push_addition(int64_t p_index) noexcept {
        #ifdef NDEBUG
            [[assume(count < capacity)]];
        #else
            assert(count < capacity && "TaskSelectionCommandPOD arena overflow!");
        #endif
        
        queued_indices[count++] = p_index;
    }

    [[nodiscard]] inline bool is_valid() const noexcept {
        return queued_indices != nullptr;
    }
    
    inline void reset() noexcept {
        count = 0;
        target_buffer_id = 0;
    }
};

static_assert(sizeof(TaskSelectionCommandPOD) == 32, "TaskSelectionCommandPOD alignment violated! Must be 32 bytes.");

} // namespace ideam::core

 // IDEAM_CORE_TASK_SELECTION_COMMAND_POD_H