#ifndef IDEAM_CORE_MEMORY_MANAGER_DOD_H
#define IDEAM_CORE_MEMORY_MANAGER_DOD_H

#include "memory_buffer_pod.h"
#include "memory_grant_pod.h"
#include "memory_buffer_selection_pod.h"
#include "../security/collision.h"

#include <godot_cpp/classes/rendering_device.hpp>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace ideam::core {

/**
 * GpuSyncCommand
 * Internal POD for the System Command Ring.
 */
struct GpuSyncCommand {
    uint32_t buffer_id;
    size_t offset;
    size_t size;
};

/**
 * MemoryManagerDOD
 * Centralized authority for POD memory and GPU synchronization.
 * Manages a single contiguous block (The Master Block) and provides
 * safe, versioned access to slices (MemoryBufferPOD).
 */
class MemoryManagerDOD {
private:
    // --- Master Allocation ---
    uint8_t* master_block_ptr = nullptr;
    size_t master_capacity = 0;
    size_t master_used = 0;

    // --- Versioning ---
    // Incremented on every rebase/reallocation to invalidate pointers globally.
    uint64_t global_version = 0;

    // --- The "State Tables" (Data-Oriented Tables) ---
    std::vector<MemoryBufferPOD> buffers;
    
    // Parallel tables for GPU management to keep PODs clean
    std::vector<godot::RID> buffer_gpu_rids;
    std::vector<uint8_t> buffer_dirty_flags; // Bit 0: CPU Dirty, Bit 1: GPU Dirty

    std::unordered_map<uint32_t, uint32_t> id_to_index;

    // --- Shadow Buffer Mapping ---
    // Maps a Data Buffer ID to its Metadata Shadow Buffer ID
    std::unordered_map<uint32_t, uint32_t> data_to_shadow_map;

    // --- Concurrency Tracking ---
    // Tracks active writers via bitset collision to prevent data races.
    std::unordered_map<uint32_t, std::vector<uint64_t>> active_write_masks;
    
    // --- Internal System Buffers ---
    uint32_t system_command_ring_id = 0xFFFFFFFF;

    // --- Godot Integration ---
    godot::RenderingDevice* rd = nullptr;
    std::mutex manager_mutex;

    // Internal Helpers
    void _update_buffer_pointers();
    void _sync_buffer_to_vram(uint32_t p_index);
    void _initialize_system_buffers();
    void _resolve_selection_pointers(uint32_t p_data_buffer_id, MemoryBufferSelectionPOD& r_selection);
    
    /**
     * _check_and_apply_selection_to_mask
     * Performs intersection checks for WRITE access and updates the active mask.
     * @return true if a collision was detected (for p_add = true), false otherwise.
     */
    bool _check_and_apply_selection_to_mask(const MemoryBufferSelectionPOD& p_selection, uint64_t* p_mask, bool p_add);

public:
    MemoryManagerDOD(size_t p_initial_capacity);
    ~MemoryManagerDOD();

    void set_rendering_device(godot::RenderingDevice* p_rd) { rd = p_rd; }

    // --- Buffer Lifecycle ---
    /**
     * create_buffer
     * Carves a slice out of the master block.
     * @return Unique ID for the buffer, or 0xFFFFFFFF on failure.
     */
    uint32_t create_buffer(BufferLayoutType p_layout, size_t p_size_bytes, uint32_t p_alignment = 64);
    
    /**
     * create_shadowed_buffer
     * Creates a primary buffer and a shadow buffer for metadata/selection storage.
     * @return Unique ID for the PRIMARY buffer.
     */
    uint32_t create_shadowed_buffer(BufferLayoutType p_layout, size_t p_size_bytes, int64_t p_max_elements, SelectionMode p_selection_mode);

    /**
     * configure_buffer_columns
     * Defines the internal structure of a buffer (AoS strides or SoA offsets).
     */
    void configure_buffer_columns(uint32_t p_id, const std::vector<ColumnMetadata>& p_columns);

    /**
     * configure_tiled_soa
     * Sets tile parameters for TILED_SOA layouts.
     */
    void configure_tiled_soa(uint32_t p_id, uint32_t p_elements_per_tile);

    /**
     * configure_paged
     * Initializes the page table for PAGED layouts.
     */
    void configure_paged(uint32_t p_id, uint32_t p_page_size_bytes);

    // --- Ring Operations ---
    bool ring_push(uint32_t p_id, const void* p_data, size_t p_size);
    bool ring_pop(uint32_t p_id, void* r_dest, size_t p_size);

    // --- Grant & Synchronization ---
    /**
     * bake_grant
     * Resolves requirements into high-performance pointers. 
     * Performs JIT GPU sync if p_needs_gpu is true.
     */
    bool bake_grant(MemoryGrantPOD& r_grant, const std::vector<GrantPartPOD>& p_requirements, bool p_needs_gpu = false);
    
    /**
     * release_grant
     * Clears write masks and marks data as dirty for future GPU syncs.
     */
    void release_grant(MemoryGrantPOD& r_grant);

    /**
     * flush_gpu_updates
     * Manual trigger to push all "CPU-Dirty" buffers to VRAM in a batch.
     * Drains the internal Command Ring.
     */
    void flush_gpu_updates();

    // --- System Logic ---
    void rebase_master_block(size_t p_new_capacity);
    
    // Getters
    [[nodiscard]] const uint64_t* get_global_version_ptr() const { return &global_version; }
    [[nodiscard]] MemoryBufferPOD* get_buffer(uint32_t p_id);
    [[nodiscard]] godot::RenderingDevice* get_rendering_device() const { return rd; }

};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_MANAGER_DOD_H