#ifndef IDEAM_CORE_MULTI_ELEMENT_VIEW_H
#define IDEAM_CORE_MULTI_ELEMENT_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * MultiElementView<Strategy>
 * A specialized view for high-performance multi-member access within a single buffer.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Facilitates "plucking" multiple members from an element base via raw offsets.
 */
template<typename Strategy = AoSStrategy>
struct MultiElementView {
    // --- 8-Byte Block ---
    uint8_t* head_ptr = nullptr; 
    const MemoryGrantPOD* grant = nullptr;

    // --- 4-Byte Block ---
    uint32_t grant_part_index = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Strategy Policy ---
    [[no_unique_address]] Strategy strategy;

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::RANDOM_ACCESS | 
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
     * operator[]
     * Selection-Relative Linear Access.
     * Returns the raw byte pointer for an element based on the current selection.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline uint8_t* operator[](size_t p_selection_index) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        if (p_selection_index >= static_cast<size_t>(selection.element_count)) {
            return head_ptr;
        }

        size_t actual_buffer_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE:
                actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                break;
            case SelectionMode::DENSE:
                actual_buffer_index = p_selection_index;
                break;
            case SelectionMode::RANGE:
                actual_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                break;
        }

        // Final safety check against selection bitmask for DENSE mode
        if (!selection.is_selected(static_cast<int64_t>(actual_buffer_index))) {
            return head_ptr;
        }

        return strategy.resolve(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
    }

    /**
     * at_base
     * Returns the raw byte pointer for an element at (x, y, [z], [w]).
     * Guarded by Selection check.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline uint8_t* at_base(Args... p_coords) const {
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
            return head_ptr;
        }

        return strategy.resolve(head_ptr, static_cast<size_t>(flat_idx), part.element_stride, part.capacity_bytes);
    }

    /**
     * pluck<T>
     * Helper to retrieve a specific member from an element base.
     */
    template<typename T>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T& pluck(uint8_t* p_element_base, size_t p_member_offset) {
        return *reinterpret_cast<T*>(p_element_base + p_member_offset);
    }

    /**
     * size
     */
    [[nodiscard]] inline size_t size() const {
        return static_cast<size_t>(grant->parts[grant_part_index].selection.element_count);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MULTI_ELEMENT_VIEW_H