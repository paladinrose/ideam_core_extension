#ifndef IDEAM_CORE_PAGED_VIEW_H
#define IDEAM_CORE_PAGED_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * PagedView<T, Strategy>
 * Optimized for virtualized/sparse buffers using a page table.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 */
template<typename T, typename Strategy = FlatStrategy>
struct PagedView {
    // --- 8-Byte Block ---
    uint8_t** page_table = nullptr; 
    const MemoryGrantPOD* grant = nullptr;

    // --- 4-Byte Block ---
    uint32_t grant_part_index = 0;
    uint32_t page_shift = 0;
    uint32_t page_mask = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Strategy Policy ---
    [[no_unique_address]] Strategy strategy;

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::RANDOM_ACCESS | 
        ViewCapability::VIRTUAL_MEMORY |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr bool is_spatial = Strategy::is_spatial;
    static constexpr bool is_simd = false;
    static constexpr uint32_t lane_width = 1;

    /**
     * is_valid
     * Standard reactive version check.
     */
    [[nodiscard]] inline bool is_valid() const {
        if (!grant || !grant->active) return false;
        if (grant_part_index >= grant->part_count) return false;

        const auto& part = grant->parts[grant_part_index];
        if (part.buffer_version_at_issue != baked_buffer_version) return false;
        if (grant->global_manager_version_ptr && *grant->global_manager_version_ptr != baked_manager_version) return false;
        
        return part.selection.is_valid();
    }

    /**
     * at
     * Resolves spatial coordinates (up to 4D) to a virtualized element reference.
     * Guarded by Selection check.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& at(Args... p_coords) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;
        int64_t flat_idx = 0;

        if constexpr (sizeof...(Args) == 1) {
            flat_idx = static_cast<int64_t>(p_coords...);
        } else if constexpr (sizeof...(Args) == 2) {
            flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 3) {
            flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 4) {
            flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
        }

        if (!selection.is_selected(flat_idx)) {
            return *reinterpret_cast<T*>(page_table[0]);
        }

        const size_t byte_offset = static_cast<size_t>(flat_idx) * part.element_stride;
        const size_t page_idx = byte_offset >> page_shift;
        const size_t local_offset = byte_offset & page_mask;
        
        return *reinterpret_cast<T*>(page_table[page_idx] + local_offset);
    }

    /**
     * operator[]
     * Selection-Relative Linear Access.
     * Iterates linearly over the selection set across page boundaries.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& operator[](size_t p_selection_index) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        if (p_selection_index >= static_cast<size_t>(selection.element_count)) {
            return *reinterpret_cast<T*>(page_table[0]);
        }

        size_t actual_buffer_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE:
                actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                break;
            case SelectionMode::DENSE:
                if (!selection.is_selected(static_cast<int64_t>(p_selection_index))) {
                    return *reinterpret_cast<T*>(page_table[0]);
                }
                actual_buffer_index = p_selection_index;
                break;
            case SelectionMode::RANGE:
                actual_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                break;
        }

        const size_t byte_offset = actual_buffer_index * part.element_stride;
        const size_t page_idx = byte_offset >> page_shift;
        const size_t local_offset = byte_offset & page_mask;

        return *reinterpret_cast<T*>(page_table[page_idx] + local_offset);
    }

    /**
     * get_page
     * Returns a direct pointer to the start of a specific virtual page.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T* get_page(size_t p_page_index) const {
        return reinterpret_cast<T*>(page_table[p_page_index]);
    }

    /**
     * elements_per_page
     */
    [[nodiscard]] inline size_t elements_per_page() const {
        const auto& part = grant->parts[grant_part_index];
        return (1ULL << page_shift) / part.element_stride;
    }

    /**
     * page_count
     */
    [[nodiscard]] inline size_t page_count() const {
        const auto& part = grant->parts[grant_part_index];
        return (part.capacity_bytes + (1ULL << page_shift) - 1) >> page_shift;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_PAGED_VIEW_H