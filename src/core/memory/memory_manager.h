#ifndef IDEAM_CORE_MEMORY_MANAGER_H
#define IDEAM_CORE_MEMORY_MANAGER_H

#include "i_memory_buffer.h"
#include "memory_grant.h"
#include "../security/collision.h"
#include <godot_cpp/classes/rendering_device.hpp>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <mutex>

namespace ideam::core {

/**
 * MemoryManager
 * The central allocator and security authority for contiguous memory access.
 * Manages a single contiguous block of memory and enforces access grants via bitset collision.
 * Integrates with Godot's RenderingDevice for On-Demand GPU synchronization.
 */
class MemoryManager {
private:
    // --- Memory Lifecycle ---
    uint8_t* master_block = nullptr;
    size_t master_block_size = 0;
    uint32_t current_version = 0; // Incremented on structural changes (reallocations)
    
    std::vector<IMemoryBuffer*> registry;

    // --- GPU Backend ---
    godot::RenderingDevice* rd = nullptr;

    // --- Grant & Security State ---
    struct BufferState {
        uint32_t buffer_id;
        
        // GPU Mirroring State
        godot::RID gpu_rid;      // The Storage Buffer RID on the GPU
        bool dirty_gpu = true;   // Data has changed on CPU, needs upload
        bool dirty_cpu = false;  // Data has changed on GPU, needs download

        std::vector<uint64_t> write_mask; // Unified bitset of all active writers
        uint32_t active_readers = 0;
        bool has_writer = false;
    };

    std::vector<BufferState> buffer_states;
    std::vector<MemoryGrant*> grant_pool;
    std::mutex manager_mutex;

    // --- Internal Helpers ---
    void _recalculate_layout();
    void _free_master_block();
    BufferState* _get_or_create_state(uint32_t p_buffer_id);
    
    // Internal GPU Management
    void _sync_to_vram(BufferState& p_state, IMemoryBuffer* p_buffer);
    IMemoryBuffer* _find_buffer(uint32_t p_buffer_id);

public:
    MemoryManager();
    ~MemoryManager();

    /**
     * set_rendering_device
     * Configures the manager to use Godot's GPU backend for on-demand mirroring.
     */
    void set_rendering_device(godot::RenderingDevice* p_rd) { rd = p_rd; }

    // --- Buffer Lifecycle ---
    void register_buffer(IMemoryBuffer* p_buffer);
    void unregister_buffer(IMemoryBuffer* p_buffer);

    /**
     * request_reallocation
     * Triggers a full master-block resize and layout recalculation.
     * Increments current_version, invalidating all active grants.
     */
    void request_reallocation();

    // --- Grant API ---
    /**
     * request_grant
     * Validates a batch of buffer requirements against current active grants.
     * Returns a pooled MemoryGrant if successful, nullptr if a collision occurs.
     * @param p_needs_gpu If true, ensures GPU buffers exist and data is mirrored to VRAM.
     */
    MemoryGrant* request_grant(const std::vector<GrantPart>& p_requirements, bool p_needs_gpu = false);

    /**
     * release_grant
     * Returns a grant to the pool and clears its associated bitsets from the buffer states.
     * Handles UniformSet cleanup if the grant was GPU-active.
     */
    void release_grant(MemoryGrant* p_grant);

    // --- Metadata ---
    [[nodiscard]] size_t get_total_allocated() const { return master_block_size; }
    [[nodiscard]] uint8_t* get_master_ptr() { return master_block; }
    [[nodiscard]] uint32_t get_version() const { return current_version; }
    [[nodiscard]] const uint32_t* get_version_ptr() const { return &current_version; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_MANAGER_H