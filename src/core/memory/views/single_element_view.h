#pragma once

#include "../memory_common.h"
#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <type_traits>
#include <cstdint>
#include <cassert>

namespace ideam::core {

/**
 * SingleElementView<T, Strategy>
 * A version-secured, selection-bound lens into a specific GrantPart.
 * Access is strictly limited to the elements defined in the MemoryBufferSelectionPOD.
 * * [C++26 Enabled]: Utilizes Multidimensional Subscripts and [[assume]] attributes.
 */
template<typename T, IsMemoryStrategy Strategy = FlatStrategy>
struct SingleElementView {
    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;

    // --- 4-Byte Block ---
    uint32_t grant_part_index = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // Zero-overhead abstraction. If Strategy is empty, it adds 0 bytes to the struct size.
    #if defined(_MSC_VER)
        [[msvc::no_unique_address]] Strategy strategy;
    #else
        [[no_unique_address]] Strategy strategy;
    #endif

    // --- Explicit Alignment Padding ---
    // Locks the base members to exactly 32 bytes (half a cache line).
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
     * Validates the view against the grant and the global manager state.
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
    }
    
    /**
     * operator[] (C++23/26 Multidimensional Subscript)
     * Unified access for both 1D Linear arrays and ND Spatial Grids.
     * * Usage: 
     * view[index]       // 1D Linear
     * view[x, y, z]     // 3D Spatial
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

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            // C++26 Optimizer Hinting
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "SingleElementView out of bounds!");
            #endif

            size_t actual_buffer_index = 0;
            
            switch (selection.mode) {
                case SelectionMode::SPARSE: {
                    actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                    break;
                }
                case SelectionMode::DENSE: {
                    #ifdef NDEBUG
                        [[assume(selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
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
        // --- N-DIMENSIONAL SPATIAL ACCESS PATH ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            int64_t flat_idx = 0;
            
            if constexpr (sizeof...(Coords) == 2) {
                flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
            }

            // C++26 Optimizer Hinting: Unleashes autovectorization by guaranteeing spatial validity
            #ifdef NDEBUG
                [[assume(selection.is_selected(flat_idx))]];
            #else
                assert(selection.is_selected(flat_idx) && "Spatial access outside selection mask!");
            #endif

            if constexpr (sizeof...(Coords) == 2) {
                return *strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                return *strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                return *strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            }
        } 
        // --- COMPILER TRAP FOR INVALID DIMENSIONS ---
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions provided for the assigned View Strategy!");
        }
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(SingleElementView<int, FlatStrategy>) == 32, "SingleElementView base layout alignment failed!");
#endif

} // namespace ideam::core

 // IDEAM_CORE_SINGLE_ELEMENT_VIEW_H