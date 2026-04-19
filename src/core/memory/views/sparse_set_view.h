#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * SparseSetView<T, Strategy>
 * Hybrid ECS layout. O(1) ID lookups + Cache-Friendly O(N) iteration.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * * [C++26 Enabled]: Multidimensional Subscripts and [[assume]] attributes.
 */
template<typename T, IsMemoryStrategy Strategy = FlatStrategy>
struct SparseSetView {
    // --- 8-Byte Block (32 Bytes) ---
    T* data_ptr = nullptr;           // Primary Data (Packed/Dense)
    uint32_t* sparse_ptr = nullptr;  // Secondary Index (Sparse)
    uint32_t* dense_ptr = nullptr;   // Tertiary ID List (Dense)
    const MemoryGrantPOD* grant = nullptr;

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
     * Binds the raw memory block to the View's dense data array.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void bind(const GrantPartPOD* p_part) noexcept {
        data_ptr = reinterpret_cast<T*>(p_part->raw_base_ptr);
        
        // If your allocator provides sparse_ptr and dense_ptr from different GrantPartPODs,
        // you will need to map them inside a bind_secondary() method triggered by your Logic.
    }
    
    /**
     * operator[] (C++23/26 Multidimensional Subscript)
     * 1D: Cache-friendly Dense Iteration.
     * ND: Spatial Entity ID Lookup.
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

        // --- 1D LINEAR (DENSE ITERATION) ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));

            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "SparseSetView out of bounds!");
            #endif

            size_t actual_dense_index = 0;
            switch (selection.mode) {
                case SelectionMode::SPARSE:
                    actual_dense_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                    break;
                case SelectionMode::DENSE:
                    #ifdef NDEBUG
                        [[assume(selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
                    actual_dense_index = p_selection_index;
                    break;
                case SelectionMode::RANGE:
                    actual_dense_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                    break;
            }

            if constexpr (std::is_empty_v<Strategy>) {
                return *Strategy::template resolve<T>(data_ptr, actual_dense_index, part.element_stride, part.capacity_bytes);
            } else {
                return *strategy.template resolve<T>(data_ptr, actual_dense_index, part.element_stride, part.capacity_bytes);
            }
        }
        // --- N-DIMENSIONAL SPATIAL ACCESS (SPARSE LOOKUP) ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            int64_t spatial_entity_id = 0;
            
            if constexpr (sizeof...(Coords) == 2) {
                spatial_entity_id = strategy.get_index_2d(p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                spatial_entity_id = strategy.get_index_3d(p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                spatial_entity_id = strategy.get_index_4d(p_coords..., part.element_stride);
            }

            // O(1) Lookup: Map spatial Entity ID to Dense Index
            uint32_t dense_idx = sparse_ptr[spatial_entity_id];

            #ifdef NDEBUG
                [[assume(selection.is_selected(dense_idx))]];
            #else
                assert(selection.is_selected(dense_idx) && "Spatial ID is not within the current selection!");
            #endif

            if constexpr (std::is_empty_v<Strategy>) {
                return *Strategy::template resolve<T>(data_ptr, dense_idx, part.element_stride, part.capacity_bytes);
            } else {
                return *strategy.template resolve<T>(data_ptr, dense_idx, part.element_stride, part.capacity_bytes);
            }
        }
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions for SparseSetView Strategy!");
        }
    }

    /**
     * by_id
     * Direct O(1) Sparse Lookup for a specific Entity ID.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& by_id(uint32_t p_entity_id) const noexcept {
        uint32_t dense_idx = sparse_ptr[p_entity_id];
        const auto& part = grant->parts[grant_part_index];

        #ifdef NDEBUG
            [[assume(part.selection.is_selected(dense_idx))]];
        #else
            assert(part.selection.is_selected(dense_idx) && "Entity ID is not in the active selection!");
        #endif

        if constexpr (std::is_empty_v<Strategy>) {
            return *Strategy::template resolve<T>(data_ptr, dense_idx, part.element_stride, part.capacity_bytes);
        } else {
            return *strategy.template resolve<T>(data_ptr, dense_idx, part.element_stride, part.capacity_bytes);
        }
    }

    /**
     * get_entity_at
     * Returns the global Entity ID at a specific selection index (useful during dense iteration).
     */
    inline uint32_t get_entity_at(size_t p_selection_index) const noexcept {
        const auto& selection = grant->parts[grant_part_index].selection;
        
        #ifdef NDEBUG
            [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
        #else
            assert(p_selection_index < static_cast<size_t>(selection.element_count) && "Out of bounds entity lookup!");
        #endif

        size_t actual_dense_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE: actual_dense_index = static_cast<size_t>(selection.data.indices[p_selection_index]); break;
            case SelectionMode::DENSE:  actual_dense_index = p_selection_index; break;
            case SelectionMode::RANGE:  actual_dense_index = static_cast<size_t>(selection.start_index) + p_selection_index; break;
        }

        return dense_ptr[actual_dense_index];
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(SparseSetView<int, FlatStrategy>) == 48, "SparseSetView base layout alignment failed!");
#endif

} // namespace ideam::core

 // IDEAM_CORE_SPARSE_SET_VIEW_H