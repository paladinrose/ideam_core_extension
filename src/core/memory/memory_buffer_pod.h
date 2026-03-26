#ifndef IDEAM_CORE_MEMORY_BUFFER_POD_H
#define IDEAM_CORE_MEMORY_BUFFER_POD_H

#include "memory_common.h"

namespace ideam::core {

#define MAX_BUFFER_COLUMNS 16

/**
 * BufferLayoutType
 * Identifies the structural strategy of the memory block.
 */
enum class BufferLayoutType : uint8_t {
    FLAT,       // Single contiguous array
    AOS,        // Array of Structures (Interleaved)
    SOA,        // Structure of Arrays (Parallel Lanes)
    SPARSE_SET, // Sparse/Dense/Data arrays (ECS style)
    TILED_SOA,  // Blocks of SoA for cache locality/spatial partitioning
    RING,       // Circular buffer for producer/consumer streaming
    PAGED       // Virtualized buffer via pointer table to non-contiguous blocks
};

/**
 * ColumnMetadata
 * Unified representation for AoS members, SoA columns, or SparseSet pools.
 */
struct ColumnMetadata {
    size_t offset;            // Relative to raw_ptr
    size_t secondary_offset;  // Used by SparseSet for Dense array
    size_t tertiary_offset;   // Used by SparseSet for Data array
    uint32_t id;
    uint32_t type_size;
    uint32_t alignment;
    int32_t current_size;     // Current element count in this specific pool
};

/**
 * MemoryBufferPOD
 * The definitive DOD structure.
 */
struct MemoryBufferPOD {
    // --- Core Header ---
    uint8_t* master_block_ptr;
    size_t memory_offset;
    size_t capacity_bytes;
    size_t used_bytes;
    size_t alignment_requirement;

    // --- Unified Structural Data ---
    int64_t max_elements;    
    int64_t current_count;  

     // --- Layout-Specific Metadata ---
    union {
        // TILED_SOA: Size of one block of elements before the next attribute starts
        struct {
            uint32_t elements_per_tile; 
            uint32_t tile_stride_bytes;
        } tiled;

        // RING: Head/Tail offsets for streaming
        struct {
            size_t head_offset;
            size_t tail_offset;
            bool is_full;
        } ring;

        // PAGED: Pointer to the page table (array of uint8_t*)
        struct {
            uint8_t** table_ptr;
            uint32_t page_count;
            uint32_t page_size_bytes;
        } paged;
    } extra;
    
    uint32_t buffer_id;
    uint32_t version;
    uint32_t element_stride; 
    uint32_t column_count;

    BufferLayoutType layout_type;
    BufferAlignmentMode alignment_mode;
    BufferLifecycleState lifecycle;

    bool needs_compaction;   // For SoA death_row logic

    ColumnMetadata columns[MAX_BUFFER_COLUMNS];

};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_BUFFER_POD_H