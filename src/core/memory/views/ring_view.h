#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * RingView<T, Strategy>
 * Specialized for circular buffer streaming with optional spatial addressing.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Selection logic is applied to the physical buffer index after the read-head offset is calculated.
 * * [C++26 Enabled]: Multidimensional Subscripts, [[assume]] optimizations, and perfect 48-byte alignment.
 */
template<typename T, IsMemoryStrategy Strategy = FlatStrategy>
struct RingView {
    // --- 8-Byte Block (32 Bytes) ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;
    uint32_t* read_index_ptr = nullptr;
    uint32_t* write_index_ptr = nullptr;

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
    // Locks the base members to exactly 48 bytes (perfect 16-byte multiple).
    uint8_t reserved_padding[4] = {0};

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
     * Handles both 1D Stream polling and ND Spatial Ring mapping.
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

        size_t logical_idx = 0;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "RingView logical index out of bounds!");
            #endif

            switch (selection.mode) {
                case SelectionMode::SPARSE:
                    logical_idx = static_cast<size_t>(selection.data.indices[p_selection_index]);
                    break;
                case SelectionMode::DENSE:
                    #ifdef NDEBUG
                        [[assume(selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
                    logical_idx = p_selection_index;
                    break;
                case SelectionMode::RANGE:
                    logical_idx = static_cast<size_t>(selection.start_index) + p_selection_index;
                    break;
            }
        } 
        // --- N-DIMENSIONAL SPATIAL ACCESS PATH ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            if constexpr (sizeof...(Coords) == 2) {
                logical_idx = static_cast<size_t>(strategy.get_index_2d(p_coords..., part.element_stride));
            } else if constexpr (sizeof...(Coords) == 3) {
                logical_idx = static_cast<size_t>(strategy.get_index_3d(p_coords..., part.element_stride));
            } else if constexpr (sizeof...(Coords) == 4) {
                logical_idx = static_cast<size_t>(strategy.get_index_4d(p_coords..., part.element_stride));
            }
        } 
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions for RingView Strategy!");
        }

        // Apply read-head offset for circular hardware wrap-around
        const size_t physical_idx = (*read_index_ptr + logical_idx) % selection.capacity;

        // C++26 Optimizer Hinting: Guarantee the wrapped physical index remains within the bitmask
        #ifdef NDEBUG
            [[assume(selection.is_selected(static_cast<int64_t>(physical_idx)))]];
        #else
            assert(selection.is_selected(static_cast<int64_t>(physical_idx)) && "Ring physical index is outside selection mask!");
        #endif

        if constexpr (std::is_empty_v<Strategy>) {
            return *Strategy::template resolve<T>(head_ptr, physical_idx, part.element_stride, part.capacity_bytes);
        } else {
            return *strategy.template resolve<T>(head_ptr, physical_idx, part.element_stride, part.capacity_bytes);
        }
    }

    /**
     * available
     * Returns the number of unread elements currently in the ring.
     */
    [[nodiscard]] inline size_t available() const noexcept {
        if (*write_index_ptr >= *read_index_ptr) {
            return *write_index_ptr - *read_index_ptr;
        }
        const auto& part = grant->parts[grant_part_index];
        return (static_cast<size_t>(part.selection.capacity) - *read_index_ptr) + *write_index_ptr;
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(RingView<int, FlatStrategy>) == 48, "RingView base layout alignment failed!");
#endif

} // namespace ideam::core

 // IDEAM_CORE_MEMORY_RING_VIEW_H