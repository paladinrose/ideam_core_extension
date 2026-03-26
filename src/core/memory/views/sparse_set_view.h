#ifndef IDEAM_CORE_SPARSE_SET_VIEW_H
#define IDEAM_CORE_SPARSE_SET_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * SparseSetView<T, Strategy>
 * Hybrid ECS layout. O(1) ID lookups + Cache-Friendly O(N) iteration.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Selection logic is applied to the DENSE index for iteration and the SPARSE lookup for spatial/ID access.
 */
template<typename T, typename Strategy = FlatStrategy>
struct SparseSetView {
    // --- 8-Byte Block ---
    T* data_ptr = nullptr;           // Primary Data (Packed/Dense)
    uint32_t* sparse_ptr = nullptr;  // Secondary Index (Sparse)
    uint32_t* dense_ptr = nullptr;   // Tertiary ID List (Dense)
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
     * contains
     * Checks if a global Entity ID exists within this sparse set and is selected.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool contains(uint32_t p_entity_id) const {
        const auto& part = grant->parts[grant_part_index];
        const uint32_t dense_idx = sparse_ptr[p_entity_id];
        
        // Basic sparse set validation
        bool exists = dense_idx < static_cast<uint32_t>(part.element_count) && dense_ptr[dense_idx] == p_entity_id;
        if (!exists) return false;

        // Selection validation: Is the dense index part of the current selection?
        return part.selection.is_selected(static_cast<int64_t>(dense_idx));
    }

    /**
     * at (Spatial Lookup)
     * Translates coordinates to an ID via Strategy (applied to sparse array), then retrieves the data.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& at(Args... p_coords) const {
        const auto& part = grant->parts[grant_part_index];
        
        // 1. Resolve coordinates to a physical Entity ID via the strategy acting on the sparse array.
        uint32_t* id_ptr = nullptr;
        if constexpr (sizeof...(Args) == 1) {
            id_ptr = strategy.resolve(sparse_ptr, static_cast<size_t>(p_coords...), sizeof(uint32_t), part.capacity_bytes);
        } else if constexpr (sizeof...(Args) == 2) {
            id_ptr = strategy.resolve_2d(sparse_ptr, p_coords..., sizeof(uint32_t));
        } else if constexpr (sizeof...(Args) == 3) {
            id_ptr = strategy.resolve_3d(sparse_ptr, p_coords..., sizeof(uint32_t));
        } else if constexpr (sizeof...(Args) == 4) {
            id_ptr = strategy.resolve_4d(sparse_ptr, p_coords..., sizeof(uint32_t));
        }

        // 2. Retrieve via the Entity ID found at that location
        return get_by_id(*id_ptr);
    }

    /**
     * get_by_id
     * Returns the data for a specific Entity ID, guarded by Selection.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& get_by_id(uint32_t p_entity_id) const {
        const auto& part = grant->parts[grant_part_index];
        const uint32_t dense_idx = sparse_ptr[p_entity_id];

        if (!part.selection.is_selected(static_cast<int64_t>(dense_idx))) {
            return *data_ptr;
        }

        return data_ptr[dense_idx];
    }

    /**
     * operator[]
     * Selection-Relative Linear Access.
     * Accesses data by its DENSE index relative to the SelectionPOD.
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
            return *data_ptr;
        }

        size_t actual_dense_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE:
                actual_dense_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                break;
            case SelectionMode::DENSE:
                actual_dense_index = p_selection_index;
                break;
            case SelectionMode::RANGE:
                actual_dense_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                break;
        }

        // Redundant check for DENSE mode to ensure selection bitmask compliance
        if (!selection.is_selected(static_cast<int64_t>(actual_dense_index))) {
            return *data_ptr;
        }

        return data_ptr[actual_dense_index];
    }

    /**
     * get_entity_at
     * Returns the global Entity ID at a specific selection index.
     */
    inline uint32_t get_entity_at(size_t p_selection_index) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        if (p_selection_index >= static_cast<size_t>(selection.element_count)) {
            return 0; // Reserved/Null ID
        }

        size_t actual_dense_index = 0;
        if (selection.mode == SelectionMode::SPARSE) {
            actual_dense_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
        } else if (selection.mode == SelectionMode::RANGE) {
            actual_dense_index = static_cast<size_t>(selection.start_index) + p_selection_index;
        } else {
            actual_dense_index = p_selection_index;
        }

        return dense_ptr[actual_dense_index];
    }

    /**
     * size
     * Returns the number of selected elements currently visible.
     */
    [[nodiscard]] inline size_t size() const {
        return static_cast<size_t>(grant->parts[grant_part_index].selection.element_count);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_SPARSE_SET_VIEW_H