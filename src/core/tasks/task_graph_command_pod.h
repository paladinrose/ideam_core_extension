#ifndef IDEAM_CORE_TASK_GRAPH_COMMAND_POD_H
#define IDEAM_CORE_TASK_GRAPH_COMMAND_POD_H

#include <cstdint>

namespace ideam::core {

/**
 * TaskGraphCommandPOD
 * A raw byte bump-allocator for deferring Tier 1 Physical Extensions
 * (e.g., spawning elements, buffer resizing) to the end of the graph cycle.
 */
struct TaskGraphCommandPOD {
    uint8_t* arena_ptr = nullptr;
    uint32_t write_offset = 0;
    uint32_t capacity = 0;

    /**
     * push_command
     * Serializes an arbitrary POD struct into the Tier 1 buffer.
     */
    template <typename T>
    inline void push_command(const T& p_cmd) noexcept {
        if (write_offset + sizeof(T) <= capacity) {
            *(reinterpret_cast<T*>(arena_ptr + write_offset)) = p_cmd;
            write_offset += sizeof(T);
        }
    }
    
    inline void reset() noexcept {
        write_offset = 0;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_TASK_GRAPH_COMMAND_POD_H