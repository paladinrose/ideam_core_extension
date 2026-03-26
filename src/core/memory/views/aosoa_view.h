#ifndef IDEAM_CORE_AOSOA_VIEW_H
#define IDEAM_CORE_AOSOA_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * AOSOAView<T, LaneWidth, Strategy>
 * Represents an "Array of Structures of Arrays" layout.
 * Optimized for SIMD kernels, balancing spatial locality with hardware alignment.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 */
template<typename T, uint32_t LaneWidth = 8, typename Strategy = FlatStrategy>
struct AOSOAView {
    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
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
        ViewCapability::SIMD_ACCESS |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr bool is_spatial = Strategy::is_spatial;
    static constexpr bool is_simd = true;
    static constexpr uint32_t lane_width = LaneWidth;

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
     * get_lane
     * Returns a pointer to the start of a SIMD lane.
     * Access is strictly guarded by the Selection set. If the base coordinate 
     * or lane start is not selected, returns head_ptr as an access violation guard.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    template<typename... Args>
    inline T* get_lane(size_t p_lane_index, Args... p_coords) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;
        T* base_ptr = nullptr;
        int64_t base_flat_idx = 0;

        // Resolve base and logical index via Strategy
        if constexpr (sizeof...(Args) == 0) {
            base_ptr = head_ptr;
            base_flat_idx = 0;
        } else if constexpr (sizeof...(Args) == 2) {
            base_ptr = strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            base_flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 3) {
            base_ptr = strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            base_flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 4) {
            base_ptr = strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            base_flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
        }

        // The effective flat index for the start of the SIMD lane
        int64_t effective_idx = base_flat_idx + static_cast<int64_t>(p_lane_index * LaneWidth);

        // Strict Selection Guard
        if (!selection.is_selected(effective_idx)) {
            return head_ptr;
        }

        return base_ptr + (p_lane_index * LaneWidth);
    }

    /**
     * lane_count
     * Returns the number of complete hardware lanes available within the selection.
     */
    [[nodiscard]] inline size_t lane_count() const {
        return static_cast<size_t>(grant->parts[grant_part_index].selection.element_count) / LaneWidth;
    }

    /**
     * operator[]
     * Selection-Relative Linear Access.
     * Maps p_selection_index to the buffer index via MemoryBufferSelectionPOD.
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
            return *head_ptr;
        }

        size_t actual_buffer_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE: {
                actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                break;
            }
            case SelectionMode::DENSE: {
                if (!selection.is_selected(static_cast<int64_t>(p_selection_index))) {
                    return *head_ptr;
                }
                actual_buffer_index = p_selection_index;
                break;
            }
            case SelectionMode::RANGE: {
                actual_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                break;
            }
        }

        if constexpr (std::is_empty_v<Strategy>) {
            return *Strategy::template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
        } else {
            return *strategy.template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_AOSOA_VIEW_H