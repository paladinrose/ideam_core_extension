#ifndef IDEAM_CORE_SWAP_VIEW_H
#define IDEAM_CORE_SWAP_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <utility>
#include <cstdint>

namespace ideam::core {

/**
 * SwapView<T, Strategy>
 * A dual-buffer view for ping-pong simulation states (Double Buffering).
 * Selection-aware: Iteration (operator[]) follows the Read-buffer's selection.
 * Ensures the 'Read' buffer (State T) is never mutated by the 'Write' operation (State T+1).
 */
template<typename T, typename Strategy = FlatStrategy>
struct SwapView {
    // --- 8-Byte Aligned Block ---
    T* read_head = nullptr;
    T* write_head = nullptr;
    const MemoryGrantPOD* read_grant = nullptr;
    const MemoryGrantPOD* write_grant = nullptr;

    // --- 4-Byte Aligned Block ---
    uint32_t read_part_idx = 0;
    uint32_t write_part_idx = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Policy ---
    [[no_unique_address]] Strategy strategy;

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::RANDOM_ACCESS | 
        ViewCapability::DUAL_BUFFER |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr bool is_spatial = Strategy::is_spatial;
    static constexpr bool is_simd = false;

    /**
     * is_valid
     * Validates both the source and destination grants.
     */
    [[nodiscard]] inline bool is_valid() const {
        if (!read_grant || !write_grant || !read_grant->active || !write_grant->active) return false;
        
        const auto& r_part = read_grant->parts[read_part_idx];
        const auto& w_part = write_grant->parts[write_part_idx];

        return (r_part.buffer_version_at_issue == baked_buffer_version &&
                w_part.buffer_version_at_issue == baked_buffer_version);
    }

    /**
     * operator[]
     * Linear access to the READ buffer (State T) via the current Selection.
     * This is the primary driver for simulation loops.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline const T& operator[](size_t p_selection_index) const {
        const auto& part = read_grant->parts[read_part_idx];
        const auto& selection = part.selection;

        if (p_selection_index >= static_cast<size_t>(selection.element_count)) {
            return *read_head;
        }

        size_t abs_idx = selection.get_absolute_index(p_selection_index);
        return *strategy.resolve(read_head, abs_idx, part.element_stride, part.capacity_bytes);
    }

    /**
     * write_to_selection
     * Mutable access to the WRITE buffer (State T+1) at the same selection-relative index.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& write_to_selection(size_t p_selection_index) const {
        const auto& r_part = read_grant->parts[read_part_idx];
        const auto& w_part = write_grant->parts[write_part_idx];
        
        // Map the read-selection index to absolute space
        size_t abs_idx = r_part.selection.get_absolute_index(p_selection_index);
        
        // Resolve in write buffer
        return *strategy.resolve(write_head, abs_idx, w_part.element_stride, w_part.capacity_bytes);
    }

    /**
     * read_at / write_at
     * Variadic spatial access for cross-stencil lookups (e.g., neighbor sampling in State T).
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline const T& read_at(Args... p_coords) const {
        const auto& part = read_grant->parts[read_part_idx];
        return *strategy.resolve_nd(read_head, part.element_stride, p_coords...);
    }

    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& write_at(Args... p_coords) const {
        const auto& part = write_grant->parts[write_part_idx];
        return *strategy.resolve_nd(write_head, part.element_stride, p_coords...);
    }

    /**
     * swap_buffers
     * Swaps the roles of the internal pointers and grants.
     */
    inline void swap_buffers() {
        std::swap(read_head, write_head);
        std::swap(read_grant, write_grant);
        std::swap(read_part_idx, write_part_idx);
    }

    /**
     * size
     * Returns the count of selected elements in the current Read state.
     */
    [[nodiscard]] inline size_t size() const {
        return static_cast<size_t>(read_grant->parts[read_part_idx].selection.element_count);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SWAP_VIEW_H