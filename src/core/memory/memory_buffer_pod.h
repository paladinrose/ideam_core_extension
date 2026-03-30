#ifndef IDEAM_CORE_MEMORY_BUFFER_POD_H
#define IDEAM_CORE_MEMORY_BUFFER_POD_H

#include "memory_common.h"
#include <cstdint>
#include <cstddef>

namespace ideam::core {

// Technical Constraint: Max Buffer Columns is 16
constexpr uint32_t MAX_BUFFER_COLUMNS = 16;

/**
 * BufferLayoutType
 * Identifies the structural strategy of the memory block.
 */
enum class BufferLayoutType : uint16_t {
    NONE       = 0,
    FLAT       = 1 << 0, // Single contiguous array
    AOS        = 1 << 1, // Array of Structures (Interleaved)
    SOA        = 1 << 2, // Structure of Arrays (Parallel Lanes)
    SPARSE_SET = 1 << 3, // Sparse/Dense/Data arrays (ECS style)
    TILED_SOA  = 1 << 4, // Blocks of SoA for cache locality
    RING       = 1 << 5, // Circular buffer 
    PAGED      = 1 << 6, // Virtualized buffer via pointer table

    // --- UI/Graph Helper Masks ---
    ANY_LINEAR   = FLAT | AOS | SOA | SPARSE_SET | TILED_SOA | RING | PAGED,
    ANY_PARALLEL = SOA | TILED_SOA,
    ANY_SPATIAL  = FLAT | SOA | TILED_SOA | PAGED 
};

constexpr BufferLayoutType operator|(BufferLayoutType a, BufferLayoutType b) noexcept {
    return static_cast<BufferLayoutType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

constexpr BufferLayoutType operator&(BufferLayoutType a, BufferLayoutType b) noexcept {
    return static_cast<BufferLayoutType>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

constexpr bool has_layout(BufferLayoutType p_mask, BufferLayoutType p_layout) noexcept {
    return (p_mask & p_layout) != BufferLayoutType::NONE;
}

/**
 * ColumnMetadata
 * Unified representation for AoS members, SoA columns, or SparseSet pools.
 * Strictly packed to 40 bytes.
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
static_assert(sizeof(ColumnMetadata) == 40, "ColumnMetadata packing violated!");

/**
 * MemoryBufferPOD
 * The definitive DOD structure defining a sliced buffer from the Master Block.
 */
struct MemoryBufferPOD {
    // --- 8-Byte Alignment Block ---
    uint8_t* master_block_ptr;
    size_t memory_offset;
    size_t capacity_bytes;
    size_t used_bytes;
    size_t alignment_requirement;

    // --- Unified Structural Data ---
    int64_t max_elements;    
    int64_t current_count;  

    // --- Layout-Specific Metadata ---
    union ExtraData {
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
    } extra; // 24 bytes total, aligned to 8-byte boundary
    
    // --- 4-Byte Alignment Block ---
    uint32_t buffer_id;
    uint32_t version;
    uint32_t element_stride; 
    uint32_t column_count;

    // --- 1-Byte Alignment Block ---
    BufferLayoutType layout_type;
    BufferAlignmentMode alignment_mode;
    BufferLifecycleState lifecycle;
    bool needs_compaction;   // For SoA death_row logic

    // --- Explicit Padding ---
    // Replaces the 4 bytes of implicit compiler padding that would normally exist 
    // before the 8-byte aligned ColumnMetadata array begins.
    uint8_t reserved_padding[3];

    // --- Arrays (8-byte aligned by offset) ---
    ColumnMetadata columns[MAX_BUFFER_COLUMNS];
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_BUFFER_POD_H