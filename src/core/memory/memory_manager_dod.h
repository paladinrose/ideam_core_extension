#ifndef IDEAM_CORE_MEMORY_MANAGER_DOD_H
#define IDEAM_CORE_MEMORY_MANAGER_DOD_H

#include "memory_buffer_pod.h"
#include "memory_grant_pod.h"
#include "memory_buffer_selection_pod.h"
#include "../security/collision.h"

#include <godot_cpp/classes/rendering_device.hpp>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <span>
#include <concepts>
#include <new>

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

// C++20 Concept: Enforces that the passed type is exactly one of our secured Grant footprints.
template <typename T>
concept IsMemoryGrant = std::is_same_v<T, MemoryGrantPOD> || std::is_same_v<T, MemoryGrantHeavyPOD>;

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
    // Flat vectors pre-reserved to prevent dynamic reallocation and pointer invalidation.
    std::vector<MemoryBufferPOD> buffers;
    std::vector<godot::RID> buffer_gpu_rids;
    std::vector<uint8_t> buffer_dirty_flags; // Bit 0: CPU Dirty, Bit 1: GPU Dirty

    // Sparse Array Map: O(1) Cache-Friendly lookups. Index == buffer_id, Value == internal vector index.
    std::vector<uint32_t> id_to_index;

    // --- Shadow Buffer Mapping ---
    // Sparse Array Map: Index == Data Buffer ID, Value == Metadata Shadow Buffer ID
    std::vector<uint32_t> data_to_shadow_map;

    // --- Concurrency Tracking ---
    // Tracks active writers via bitset collision to prevent data races.
    // Flat array of vectors. Evaluated via C++20 std::atomic_ref.
    std::vector<std::vector<uint64_t>> active_write_masks;
    
    // --- Internal System Buffers ---
    uint32_t system_command_ring_id = 0xFFFFFFFF;

    // --- Godot Integration ---
    godot::RenderingDevice* rd = nullptr;
    
    // --- Thread Safety ---
    // C++17 Read-Write Lock to allow extreme parallel read throughput on worker threads.
    std::shared_mutex manager_rw_lock;
    // Fine-grained spinlock specifically for the GPU synchronization ring buffer.
    std::mutex command_ring_mutex;

    // Internal Helpers
    void _update_buffer_pointers();
    void _sync_buffer_to_vram(uint32_t p_index);
    void _initialize_system_buffers();
    void _resolve_selection_pointers(uint32_t p_data_buffer_id, MemoryBufferSelectionPOD& r_selection);
    
    /**
     * _check_and_apply_selection_to_mask
     * Performs lock-free intersection checks for WRITE access and updates the active mask.
     * @return true if a collision was detected (for p_add = true), false otherwise.
     */
    bool _check_and_apply_selection_to_mask(const MemoryBufferSelectionPOD& p_selection, uint64_t* p_mask, bool p_add);

    /**
     * _bake_grant_core
     * The internal DOD pointer resolution logic. Decoupled from the template to prevent header bloat.
     */
    bool _bake_grant_core(GrantPartPOD* r_parts, uint32_t max_parts, uint32_t& r_part_count, uint64_t& r_uniform_handle, std::span<const GrantPartPOD> p_requirements, bool p_needs_gpu);

    /**
     * _release_grant_core
     * Internal DOD release logic.
     */
    void _release_grant_core(GrantPartPOD* p_parts, uint32_t p_part_count, uint64_t& r_uniform_handle);

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

    /**
     * expand_paged_buffer
     * Seamlessly appends new hardware pages to a PAGED buffer's table pointer without moving existing data.
     * Prevents master block fragmentation.
     */
    bool expand_paged_buffer(uint32_t p_id, size_t p_new_size_bytes);

    // --- Ring Operations ---
    bool ring_push(uint32_t p_id, const void* p_data, size_t p_size);
    bool ring_pop(uint32_t p_id, void* r_dest, size_t p_size);

    // --- Topological & Semantic Queries ---
    
    // Sparse Set Utilities
    [[nodiscard]] bool buffer_contains_id(uint32_t p_buffer_id, uint32_t p_entity_id) const;
    [[nodiscard]] int32_t get_dense_index(uint32_t p_buffer_id, uint32_t p_entity_id) const;

    // Semantic & Layout Utilities
    [[nodiscard]] bool buffer_has_column(uint32_t p_buffer_id, uint32_t p_column_id) const;
    [[nodiscard]] bool buffer_has_data_type(uint32_t p_buffer_id, DataType p_type) const;
    [[nodiscard]] int32_t get_column_offset(uint32_t p_buffer_id, uint32_t p_column_id) const;

    // Ring Buffer Utilities
    [[nodiscard]] size_t get_ring_available_read_bytes(uint32_t p_buffer_id) const;
    [[nodiscard]] size_t get_ring_available_write_bytes(uint32_t p_buffer_id) const;
    [[nodiscard]] bool is_ring_full(uint32_t p_buffer_id) const;
    [[nodiscard]] bool is_ring_empty(uint32_t p_buffer_id) const;

    // Paged Buffer Utilities
    [[nodiscard]] uint32_t get_paged_allocated_page_count(uint32_t p_buffer_id) const;
    [[nodiscard]] bool is_page_allocated_for_index(uint32_t p_buffer_id, size_t p_flat_index) const;

    // Tiled SoA Utilities
    [[nodiscard]] uint32_t get_tile_count(uint32_t p_buffer_id) const;
    [[nodiscard]] uint32_t get_elements_per_tile(uint32_t p_buffer_id) const;

    // --- Grant & Synchronization ---
    /**
     * bake_grant
     * Resolves requirements into high-performance pointers. 
     * Performs JIT GPU sync if p_needs_gpu is true.
     */
    template <IsMemoryGrant TGrant>
    bool bake_grant(TGrant& r_grant, std::span<const GrantPartPOD> p_requirements, bool p_needs_gpu = false) {
        constexpr uint32_t max_parts = sizeof(r_grant.parts) / sizeof(GrantPartPOD);
        if (p_requirements.size() > max_parts) return false;

        r_grant.active = false;
        r_grant.part_count = 0;
        
        if (_bake_grant_core(r_grant.parts, max_parts, r_grant.part_count, r_grant.uniform_set_handle, p_requirements, p_needs_gpu)) {
            r_grant.manager_version_at_issue = global_version;
            r_grant.global_manager_version_ptr = &global_version;
            r_grant.active = true;
            return true;
        }
        return false;
    }
    
    /**
     * release_grant
     * Clears write masks and marks data as dirty for future GPU syncs.
     */
    template <IsMemoryGrant TGrant>
    void release_grant(TGrant& r_grant) {
        if (!r_grant.active) return;
        _release_grant_core(r_grant.parts, r_grant.part_count, r_grant.uniform_set_handle);
        r_grant.active = false;
    }

    /**
     * populate_inverse_selection
     * Transforms an empty MemoryBufferSelectionPOD into a fully populated one by
     * inverting the buffer's global bitset via SIMD operations.
     */
    void populate_inverse_selection(uint32_t p_buffer_id, MemoryBufferSelectionPOD& r_selection);
    
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