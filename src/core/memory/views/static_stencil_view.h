#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <array>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * StaticStencilView<T, Strategy, PointCount>
 * Optimized for fixed-pattern kernels (Blur, Erosion, Game of Life).
 * Pre-calculates byte-offsets for a specific neighbor set.
 * Access is strictly bound to the MemoryBufferSelectionPOD of the underlying view.
 * * [C++26 Enabled]: Multidimensional Subscripts, [[assume]] attributes, and `mutable` cursors.
 */
template<typename T, IsMemoryStrategy Strategy, size_t PointCount>
struct StaticStencilView {
    // --- 8-Byte Block (24 Bytes) ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;
    
    // Mutable allows the cursor to move while maintaining a const-correct View API
    mutable uint8_t* center_ptr = nullptr;

    // --- 4-Byte Block (12 Bytes) ---
    uint32_t grant_part_index = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // Zero-overhead abstraction. If Strategy is empty, it adds 0 bytes to the struct size.
    #if defined(_MSC_VER)
        [[msvc::no_unique_address]] Strategy strategy;
    #else
        [[no_unique_address]] Strategy strategy;
    #endif
    
    // --- Explicit Alignment Padding (4 Bytes) ---
    // Locks the base members to exactly 40 bytes before the offset array begins.
    uint8_t reserved_padding[4] = {0};

    // --- Array Block ---
    std::array<intptr_t, PointCount> baked_offsets;

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
     * Binds the raw memory block to the View's typed primary pointer.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void bind(const GrantPartPOD* p_part) noexcept {
        head_ptr = reinterpret_cast<T*>(p_part->raw_base_ptr);
        // Note: center_ptr is left null here. It is assigned dynamically during the kernel execution.
    }
    
    /**
     * operator[] (C++23/26 Multidimensional Subscript)
     * Sets the "Center" focus of the stencil to the specified coordinates.
     * Returns a reference to the center element.
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

        T* resolved = nullptr;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "StaticStencilView out of bounds!");
            #endif

            size_t actual_buffer_index = 0;
            switch (selection.mode) {
                case SelectionMode::SPARSE:
                    actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                    break;
                case SelectionMode::DENSE:
                    #ifdef NDEBUG
                        [[assume(selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
                    actual_buffer_index = p_selection_index;
                    break;
                case SelectionMode::RANGE:
                    actual_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                    break;
            }

            if constexpr (std::is_empty_v<Strategy>) {
                resolved = Strategy::template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            } else {
                resolved = strategy.template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            }
        } 
        // --- N-DIMENSIONAL SPATIAL ACCESS PATH ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            int64_t flat_idx = 0;
            
            if constexpr (sizeof...(Coords) == 2) {
                flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
                resolved = strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
                resolved = strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
                resolved = strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            }

            #ifdef NDEBUG
                [[assume(selection.is_selected(flat_idx))]];
            #else
                assert(selection.is_selected(flat_idx) && "Spatial Stencil focus outside selection mask!");
            #endif
        } 
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions for Stencil Strategy!");
        }

        // Update the internal mutable cursor
        center_ptr = reinterpret_cast<uint8_t*>(resolved);
        return *resolved;
    }

    /**
     * center
     * Returns the currently focused element without recalculating its position.
     */
    [[nodiscard]] inline T& center() const noexcept {
        #ifndef NDEBUG
            assert(center_ptr != nullptr && "StaticStencilView: Attempted to read center before focusing via operator[]!");
        #endif
        return *reinterpret_cast<T*>(center_ptr);
    }

    /**
     * neighbor
     * Zero-cost relative access to a neighbor in the pre-baked pattern.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& neighbor(size_t p_point_index) const noexcept {
        #ifdef NDEBUG
            // Highly aggressive compiler hint: Unrolls neighbor iteration loops
            [[assume(p_point_index < PointCount)]];
            [[assume(center_ptr != nullptr)]];
        #else
            assert(p_point_index < PointCount && "Stencil neighbor index out of bounds!");
            assert(center_ptr != nullptr && "Stencil center_ptr is uninitialized!");
        #endif

        return *reinterpret_cast<T*>(center_ptr + baked_offsets[p_point_index]);
    }
};

} // namespace ideam::core

 // IDEAM_CORE_STATIC_STENCIL_VIEW_H