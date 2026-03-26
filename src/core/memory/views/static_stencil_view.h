#ifndef IDEAM_CORE_STATIC_STENCIL_VIEW_H
#define IDEAM_CORE_STATIC_STENCIL_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <array>
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * StaticStencilView<T, Strategy, PointCount>
 * Optimized for fixed-pattern kernels (Blur, Erosion, Game of Life).
 * Pre-calculates byte-offsets for a specific neighbor set.
 * Access is strictly bound to the MemoryBufferSelectionPOD of the underlying view.
 */
template<typename T, typename Strategy, size_t PointCount>
struct StaticStencilView {
    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;
    uint8_t* center_ptr = nullptr;

    // --- 4-Byte Block ---
    uint32_t grant_part_index = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Array Block ---
    std::array<intptr_t, PointCount> baked_offsets;

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
     * bake_pattern
     * Calculates the byte-jumps for a set of relative coordinates based on strategy strides.
     */
    template<typename... Args>
    void bake_pattern(size_t p_point_idx, Args... p_relative_coords) {
        if (p_point_idx >= PointCount) return;

        const auto& part = grant->parts[grant_part_index];
        const size_t stride = part.element_stride;
        auto coords = std::array<int64_t, sizeof...(Args)>{ static_cast<int64_t>(p_relative_coords)... };
        
        if constexpr (sizeof...(Args) == 1) {
            baked_offsets[p_point_idx] = coords[0] * static_cast<intptr_t>(stride);
        } else if constexpr (sizeof...(Args) == 2) {
            baked_offsets[p_point_idx] = (coords[0] * static_cast<intptr_t>(stride)) + 
                                         (coords[1] * static_cast<intptr_t>(strategy.stride_y));
        } else if constexpr (sizeof...(Args) == 3) {
            baked_offsets[p_point_idx] = (coords[0] * static_cast<intptr_t>(stride)) + 
                                         (coords[1] * static_cast<intptr_t>(strategy.stride_y)) + 
                                         (coords[2] * static_cast<intptr_t>(strategy.stride_z));
        } else if constexpr (sizeof...(Args) == 4) {
            baked_offsets[p_point_idx] = (coords[0] * static_cast<intptr_t>(stride)) + 
                                         (coords[1] * static_cast<intptr_t>(strategy.stride_y)) + 
                                         (coords[2] * static_cast<intptr_t>(strategy.stride_z)) +
                                         (coords[3] * static_cast<intptr_t>(strategy.stride_w));
        }
    }

    /**
     * focus
     * Sets the center of the stencil. Guarded by selection.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void focus(Args... p_coords) {
        const auto& part = grant->parts[grant_part_index];
        int64_t flat_idx = 0;

        if constexpr (sizeof...(Args) == 1) {
            flat_idx = static_cast<int64_t>(p_coords...);
            center_ptr = reinterpret_cast<uint8_t*>(strategy.resolve(head_ptr, static_cast<size_t>(flat_idx), part.element_stride, part.capacity_bytes));
        } else if constexpr (sizeof...(Args) == 2) {
            flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
            center_ptr = reinterpret_cast<uint8_t*>(strategy.resolve_2d(head_ptr, p_coords..., part.element_stride));
        } else if constexpr (sizeof...(Args) == 3) {
            flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
            center_ptr = reinterpret_cast<uint8_t*>(strategy.resolve_3d(head_ptr, p_coords..., part.element_stride));
        } else if constexpr (sizeof...(Args) == 4) {
            flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
            center_ptr = reinterpret_cast<uint8_t*>(strategy.resolve_4d(head_ptr, p_coords..., part.element_stride));
        }

        // If not selected, point to head_ptr (safe violation zone)
        if (!part.selection.is_selected(flat_idx)) {
            center_ptr = reinterpret_cast<uint8_t*>(head_ptr);
        }
    }

    /**
     * get_neighbor
     * Zero-math neighbor access using pre-baked offsets.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& get_neighbor(size_t p_point_idx) const {
        // Performance note: We do not selection-check individual neighbors for 
        // speed in stencil kernels. Kernel must handle boundary conditions.
        return *reinterpret_cast<T*>(center_ptr + baked_offsets[p_point_idx]);
    }

    /**
     * operator[]
     * Selection-Relative Linear Access.
     * Moves the focus to the selection index and returns the center reference.
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

        // Internal focus update
        T* resolved = strategy.resolve(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
        const_cast<StaticStencilView*>(this)->center_ptr = reinterpret_cast<uint8_t*>(resolved);
        
        return *resolved;
    }

    /**
     * center
     * Returns the currently focused element.
     */
    [[nodiscard]] inline T& center() const {
        return *reinterpret_cast<T*>(center_ptr);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_STATIC_STENCIL_VIEW_H