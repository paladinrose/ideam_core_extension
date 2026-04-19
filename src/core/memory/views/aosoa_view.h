#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * AOSOAView<T, LaneWidth, Strategy>
 * Represents an "Array of Structures of Arrays" layout.
 * Optimized for SIMD kernels, balancing spatial locality with hardware alignment.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * * [C++26 Enabled]: Utilizes Multidimensional Subscripts and [[assume]] attributes.
 */
template<typename T, uint32_t LaneWidth = 8, IsMemoryStrategy Strategy = FlatStrategy>
struct AOSOAView {
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
        ViewCapability::SIMD_ACCESS |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr bool is_spatial = Strategy::is_spatial;
    static constexpr bool is_simd = true;
    static constexpr uint32_t lane_width = LaneWidth;

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
     * get_lane
     * Returns a pointer to the start of a SIMD lane.
     * C++26 Optimizer Hinting ensures zero-cost execution and maximizes autovectorization.
     */
    template<typename... Coords>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T* get_lane(size_t p_lane_index, Coords... p_coords) const noexcept {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;
        T* base_ptr = nullptr;
        int64_t base_flat_idx = 0;

        // Resolve base and logical index via Strategy
        if constexpr (sizeof...(Coords) == 0) {
            base_ptr = head_ptr;
            base_flat_idx = 0;
        } else if constexpr (sizeof...(Coords) == 2 && Strategy::is_spatial) {
            base_ptr = strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            base_flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Coords) == 3 && Strategy::is_spatial) {
            base_ptr = strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            base_flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Coords) == 4 && Strategy::is_spatial) {
            base_ptr = strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            base_flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
        } else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinates provided to get_lane for this Strategy!");
        }

        // The effective flat index for the start of the SIMD lane
        int64_t effective_idx = base_flat_idx + static_cast<int64_t>(p_lane_index * LaneWidth);

        // C++26 Optimizer Hinting: Forces the compiler to trust the memory boundaries
        #ifdef NDEBUG
            [[assume(selection.is_selected(effective_idx))]];
        #else
            assert(selection.is_selected(effective_idx) && "SIMD get_lane accessed unselected or out-of-bounds memory!");
        #endif

        return base_ptr + (p_lane_index * LaneWidth);
    }

    /**
     * lane_count
     * Returns the number of complete hardware lanes available within the selection.
     */
    [[nodiscard]] inline size_t lane_count() const noexcept {
        return static_cast<size_t>(grant->parts[grant_part_index].selection.element_count) / LaneWidth;
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
     * Unified scalar fallback access for tail-loops or debugging.
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
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "AOSOAView out of bounds!");
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
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions provided for the assigned View Strategy!");
        }
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(AOSOAView<int, 8, FlatStrategy>) == 32, "AOSOAView base layout alignment failed!");
#endif

} // namespace ideam::core

 // IDEAM_CORE_AOSOA_VIEW_H