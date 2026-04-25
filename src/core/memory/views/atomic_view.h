#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <atomic>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * AtomicView<T, Strategy>
 * A thread-safe accessor for shared buffers.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Only supports types valid for std::atomic_ref (trivially copyable).
 * * [C++26 Enabled]: Utilizes Multidimensional Subscripts and [[assume]] attributes.
 */
template<typename T, IsMemoryStrategy Strategy = FlatStrategy>
struct AtomicView {
    static_assert(std::is_trivially_copyable_v<T>, "AtomicView requires trivially copyable types.");

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
        ViewCapability::ATOMIC_ACCESS |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);
    
    static constexpr BufferLayoutType supported_layouts = 
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA | 
        BufferLayoutType::SPARSE_SET | BufferLayoutType::TILED_SOA;

    
    static constexpr ViewStrategies supported_strategies = 
        ViewStrategies::ANY_LINEAR | ViewStrategies::ANY_SPATIAL;

    // Bounded to 8 bytes or fewer to guarantee lock-free atomic hardware instructions.
    static constexpr DataType supported_types = DataType::ANY_NUMERIC;

    static constexpr uint32_t lane_width = 1;

    /**
     * is_valid
     * Reactive version check.
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
     * Returns a C++20 std::atomic_ref for safe, zero-overhead atomic operations on raw memory.
     */
    template<typename... Coords>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline std::atomic_ref<T> operator[](Coords... p_coords) const noexcept {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "AtomicView out of bounds!");
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

            T* resolved = nullptr;
            if constexpr (std::is_empty_v<Strategy>) {
                resolved = Strategy::template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            } else {
                resolved = strategy.template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            }
            
            return std::atomic_ref<T>(*resolved);
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

            // C++26 Optimizer Hinting
            #ifdef NDEBUG
                [[assume(selection.is_selected(flat_idx))]];
            #else
                assert(selection.is_selected(flat_idx) && "Spatial Atomic access outside selection mask!");
            #endif

            T* resolved = nullptr;
            if constexpr (sizeof...(Coords) == 2) {
                resolved = strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                resolved = strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                resolved = strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            }

            return std::atomic_ref<T>(*resolved);
        } 
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions provided for the assigned View Strategy!");
        }
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(AtomicView<int, FlatStrategy>) == 32, "AtomicView base layout alignment failed!");
#endif

} // namespace ideam::core

 // IDEAM_CORE_ATOMIC_VIEW_H