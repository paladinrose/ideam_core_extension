#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * PagedView<T, Strategy>
 * Optimized for virtualized/sparse buffers using a page table.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * * [C++26 Enabled]: Multidimensional Subscripts, [[assume]] optimizations, and perfect 48-byte alignment.
 */
template<typename T, IsMemoryStrategy Strategy = FlatStrategy>
struct PagedView {
    // --- 8-Byte Block (16 Bytes) ---
    uint8_t** page_table = nullptr; 
    const MemoryGrantPOD* grant = nullptr;

    // --- 4-Byte Block (20 Bytes) ---
    uint32_t grant_part_index = 0;
    uint32_t page_shift = 0;
    uint32_t page_mask = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

     // --- Strategy Policy ---
    #if defined(_MSC_VER)
        [[msvc::no_unique_address]] Strategy strategy;
    #else
        [[no_unique_address]] Strategy strategy;
    #endif

    // --- Explicit Alignment Padding (12 Bytes) ---
    // Locks the base members to exactly 48 bytes (perfect 16-byte multiple).
    uint8_t reserved_padding[12] = {0};

   

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
    [[nodiscard]] inline bool is_valid() const noexcept {
        if (!grant || !grant->active) return false;
        if (grant_part_index >= grant->part_count) return false;

        const auto& part = grant->parts[grant_part_index];
        if (part.buffer_version_at_issue != baked_buffer_version) return false;
        if (grant->global_manager_version_ptr && *grant->global_manager_version_ptr != baked_manager_version) return false;
        
        return part.selection.is_valid();
    }

    /**
     * operator[] (C++23/26 Multidimensional Subscript)
     * Extremely fast Virtual Memory lookup. Resolves N-Dimensional spatial coordinates
     * into a flat index, then uses bitwise operations to resolve the hardware page table.
     */
    template<typename... Coords>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& operator[](Coords... p_coords) const noexcept {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        size_t target_flat_idx = 0;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            // FIX: Use a comma fold expression to extract the single pack argument safely
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "PagedView out of bounds!");
            #endif

            switch (selection.mode) {
                case SelectionMode::SPARSE:
                    target_flat_idx = static_cast<size_t>(selection.data.indices[p_selection_index]);
                    break;
                case SelectionMode::DENSE:
                    #ifdef NDEBUG
                        [[assume(selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
                    target_flat_idx = p_selection_index;
                    break;
                case SelectionMode::RANGE:
                    target_flat_idx = static_cast<size_t>(selection.start_index) + p_selection_index;
                    break;
            }
        } 
        // --- N-DIMENSIONAL SPATIAL ACCESS PATH ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            if constexpr (sizeof...(Coords) == 2) {
                target_flat_idx = static_cast<size_t>(strategy.get_index_2d(p_coords..., part.element_stride));
            } else if constexpr (sizeof...(Coords) == 3) {
                target_flat_idx = static_cast<size_t>(strategy.get_index_3d(p_coords..., part.element_stride));
            } else if constexpr (sizeof...(Coords) == 4) {
                target_flat_idx = static_cast<size_t>(strategy.get_index_4d(p_coords..., part.element_stride));
            }

            #ifdef NDEBUG
                [[assume(selection.is_selected(target_flat_idx))]];
            #else
                assert(selection.is_selected(target_flat_idx) && "Spatial access outside selection mask!");
            #endif
        } 
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions for PagedView Strategy!");
        }

        // Zero-branch Virtual Page Resolution
        const size_t byte_offset = target_flat_idx * part.element_stride;
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
    inline T* get_page(size_t p_page_index) const noexcept {
        return reinterpret_cast<T*>(page_table[p_page_index]);
    }

    /**
     * elements_per_page
     */
    [[nodiscard]] inline size_t elements_per_page() const noexcept {
        const auto& part = grant->parts[grant_part_index];
        return (1ULL << page_shift) / part.element_stride;
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(PagedView<int, FlatStrategy>) == 48, "PagedView base layout alignment failed!");
#endif

} // namespace ideam::core

 // IDEAM_CORE_PAGED_VIEW_H