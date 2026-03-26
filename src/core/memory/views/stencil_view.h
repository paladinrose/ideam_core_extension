#ifndef IDEAM_CORE_STENCIL_VIEW_H
#define IDEAM_CORE_STENCIL_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * StencilView<T, Strategy, DimCount>
 * Provides relative neighbor access with zero-cost coordinate folding up to 4D.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 */
template<typename T, typename Strategy, size_t DimCount>
struct StencilView {
    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;
    uint8_t* center_ptr = nullptr;

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
     * focus
     * Moves the stencil center to a specific coordinate. 
     * Guarded by Selection check.
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
     * offset_at (2D)
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& offset_at(int64_t p_rx, int64_t p_ry) const {
        static_assert(DimCount == 2, "2D Offset requires 2D StencilView");
        const size_t stride = grant->parts[grant_part_index].element_stride;
        return *reinterpret_cast<T*>(center_ptr + (p_rx * static_cast<intptr_t>(stride)) + 
                                     (p_ry * static_cast<intptr_t>(strategy.stride_y)));
    }

    /**
     * offset_at (3D)
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& offset_at(int64_t p_rx, int64_t p_ry, int64_t p_rz) const {
        static_assert(DimCount == 3, "3D Offset requires 3D StencilView");
        const size_t stride = grant->parts[grant_part_index].element_stride;
        return *reinterpret_cast<T*>(
            center_ptr + (p_rx * static_cast<intptr_t>(stride)) + 
            (p_ry * static_cast<intptr_t>(strategy.stride_y)) + 
            (p_rz * static_cast<intptr_t>(strategy.stride_z))
        );
    }

    /**
     * offset_at (4D)
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& offset_at(int64_t p_rx, int64_t p_ry, int64_t p_rz, int64_t p_rw) const {
        static_assert(DimCount == 4, "4D Offset requires 4D StencilView");
        const size_t stride = grant->parts[grant_part_index].element_stride;
        return *reinterpret_cast<T*>(
            center_ptr + (p_rx * static_cast<intptr_t>(stride)) + 
            (p_ry * static_cast<intptr_t>(strategy.stride_y)) + 
            (p_rz * static_cast<intptr_t>(strategy.stride_z)) +
            (p_rw * static_cast<intptr_t>(strategy.stride_w))
        );
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

        T* resolved = strategy.resolve(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
        const_cast<StencilView*>(this)->center_ptr = reinterpret_cast<uint8_t*>(resolved);
        
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

#endif // IDEAM_CORE_STENCIL_VIEW_H