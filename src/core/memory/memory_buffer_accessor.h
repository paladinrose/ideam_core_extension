#ifndef IDEAM_CORE_MEMORY_BUFFER_ACCESSOR_H
#define IDEAM_CORE_MEMORY_BUFFER_ACCESSOR_H

#include "i_memory_buffer.h"
#include "memory_buffer_selection.h"
#include "memory_common.h"
#include <vector>
#include <cstdint>

namespace ideam::core {

/**
 * MemoryBufferAccessor<T>
 * * A pre-resolved snapshot of a MemoryBufferSelection designed to eliminate 
 * hot-loop branching and topology math.
 * * DESIGN PROTOCOL:
 * 1. DENSE != CONTIGUOUS. A dense selection uses bitsets but may be scattered.
 * 2. This accessor provides three paths:
 * - Contiguous Path: Direct pointer for unbroken slabs.
 * - Masked Path: Linear pointer + bitset for SIMD-friendly processing.
 * - Sparse Path: Vector of pointers for irregular/random access.
 */
template<typename T>
class MemoryBufferAccessor {
protected:
    // --- Sparse Path ---
    // Indexing into this yields the final memory address immediately.
    std::vector<T*> resolved_pointers;

    // --- Contiguous/Masked Path ---
    T* head_ptr = nullptr;
    
    // --- State Flags ---
    bool is_contiguous = false;
    bool is_dense_masked = false;

    // --- Tracking Metadata ---
    const MemoryBufferSelection* source_selection = nullptr;
    uint64_t baked_selection_version = 0;
    uint8_t* baked_base_ptr = nullptr;

public:
    MemoryBufferAccessor() = default;
    virtual ~MemoryBufferAccessor() = default;

    /**
     * resolve_all
     * Pure virtual baking phase. Implementation-specific logic must define 
     * how IDs are translated to pointers based on the buffer's layout.
     */
    virtual void resolve_all(uint8_t* p_buffer_base, const MemoryBufferSelection* p_selection) = 0;

    /**
     * needs_regeneration
     * Determines if the cached pointers are stale due to buffer rebasing 
     * or selection set mutations.
     */
    [[nodiscard]] bool needs_regeneration(uint8_t* p_current_base) const {
        if (!source_selection) return true;
        if (source_selection->version != baked_selection_version) return true;
        if (p_current_base != baked_base_ptr) return true;
        return false;
    }

    // --- Direct Accessors ---

    /**
     * get_stream
     * Provides the raw head pointer. 
     * Valid for both CONTIGUOUS and DENSE_MASKED modes.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T* get_stream() const {
        return head_ptr;
    }

    /**
     * get_mask
     * Returns the bitset for SIMD/branch masking in DENSE_MASKED mode.
     */
    [[nodiscard]] inline const std::vector<uint64_t>& get_mask() const {
        return source_selection->bitset;
    }

    /**
     * operator[]
     * Universal access. Note: In SPARSE mode, p_idx is the index into the SELECTION,
     * not the buffer index.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& operator[](size_t p_idx) const {
        if (is_contiguous || is_dense_masked) {
            return head_ptr[p_idx];
        }
        return *resolved_pointers[p_idx];
    }

    // --- Capabilities ---

    [[nodiscard]] bool can_stream_contiguous() const { return is_contiguous; }
    [[nodiscard]] bool can_stream_masked() const { return is_dense_masked; }
    [[nodiscard]] size_t count() const { 
        if (is_contiguous || is_dense_masked) return source_selection->capacity;
        return resolved_pointers.size(); 
    }

    [[nodiscard]] const MemoryBufferSelection* get_source_selection() const {
        return source_selection;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_BUFFER_ACCESSOR_H