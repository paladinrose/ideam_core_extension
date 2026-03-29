#ifndef IDEAM_CORE_BRIDGE_VIEW_H
#define IDEAM_CORE_BRIDGE_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <array>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * BridgeMetadata
 * Static POD containing the subdivision ratios and pre-calculated strides.
 */
template<size_t DimCount>
struct BridgeMetadata {
    size_t child_spatial_strides[DimCount]; 
    uint32_t scaling_shape[DimCount];
    uint32_t child_cells_per_parent = 0;
};

/**
 * BridgeView<TParent, TChild, DimCount, Strategy>
 * A composite view linking a Parent selection to a subdivided Child buffer.
 * * [C++26 Enabled]: Hierarchical Multidimensional Subscripts and [[assume]] optimizations.
 */
template<typename TParent, typename TChild, size_t DimCount, IsMemoryStrategy Strategy = Spatial2DStrategy>
struct BridgeView {
    // --- 8-Byte Block ---
    TParent* parent_head = nullptr;
    TChild* child_head = nullptr;
    const MemoryGrantPOD* parent_grant = nullptr;
    const MemoryGrantPOD* child_grant = nullptr;

    // --- Dynamic Block (Size depends on DimCount) ---
    BridgeMetadata<DimCount> bridge_info;

    // --- 4-Byte Block ---
    uint32_t parent_part_idx = 0;
    uint32_t child_part_idx = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Strategy Policy ---
    [[no_unique_address]] Strategy strategy;

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = ViewCapability::SPATIAL_ACCESS;
    static constexpr bool is_spatial = true;
    static constexpr bool is_simd = false;
    static constexpr uint32_t lane_width = 1;

    /**
     * is_valid
     * Validates dual-buffer states and global manager alignment.
     */
    [[nodiscard]] inline bool is_valid() const noexcept {
        if (!parent_grant || !parent_grant->active || !child_grant || !child_grant->active) return false;
        
        const auto& p_part = parent_grant->parts[parent_part_idx];
        const auto& c_part = child_grant->parts[child_part_idx];

        if (p_part.buffer_version_at_issue != baked_buffer_version || 
            c_part.buffer_version_at_issue != baked_buffer_version) return false;

        if (parent_grant->global_manager_version_ptr && 
            *parent_grant->global_manager_version_ptr != baked_manager_version) return false;

        return p_part.selection.is_valid() && c_part.selection.is_valid();
    }

    /**
     * operator[] (C++23/26 Multidimensional Hierarchical Subscript)
     * Accesses a specific child element localized to a parent's block.
     * Usage: view[parent_idx, child_local_x, child_local_y]
     */
    template<typename... Coords>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline TChild& operator[](size_t p_parent_selection_idx, Coords... p_child_coords) const noexcept {
        static_assert(sizeof...(Coords) == DimCount, "Child coordinate dimensions must exactly match the BridgeView DimCount!");
        static_assert(Strategy::is_spatial, "BridgeView requires a Spatial Strategy!");

        const auto& p_part = parent_grant->parts[parent_part_idx];
        const auto& p_selection = p_part.selection;
        const auto& c_part = child_grant->parts[child_part_idx];
        const auto& c_selection = c_part.selection;

        // --- Tier 1: Parent Bounds Guarantee ---
        #ifdef NDEBUG
            [[assume(p_parent_selection_idx < static_cast<size_t>(p_selection.element_count))]];
        #else
            assert(p_parent_selection_idx < static_cast<size_t>(p_selection.element_count) && "BridgeView parent index out of bounds!");
        #endif

        size_t actual_parent_idx = 0;
        switch (p_selection.mode) {
            case SelectionMode::SPARSE: {
                actual_parent_idx = static_cast<size_t>(p_selection.data.indices[p_parent_selection_idx]);
                break;
            }
            case SelectionMode::DENSE: {
                #ifdef NDEBUG
                    [[assume(p_selection.is_selected(static_cast<int64_t>(p_parent_selection_idx)))]];
                #else
                    assert(p_selection.is_selected(static_cast<int64_t>(p_parent_selection_idx)) && "Accessed unselected DENSE parent index!");
                #endif
                actual_parent_idx = p_parent_selection_idx;
                break;
            }
            case SelectionMode::RANGE: {
                actual_parent_idx = static_cast<size_t>(p_selection.start_index) + p_parent_selection_idx;
                break;
            }
        }

        // Fast-path block arithmetic: Skip the full index resolution if possible
        size_t block_byte_offset = actual_parent_idx * bridge_info.child_cells_per_parent * c_part.element_stride;
        uint8_t* child_block_ptr = reinterpret_cast<uint8_t*>(child_head) + block_byte_offset;

        // --- Tier 2: Child Spatial Resolution ---
        int64_t child_flat_local_idx = 0;
        if constexpr (sizeof...(Coords) == 2) {
            child_flat_local_idx = strategy.get_index_2d(p_child_coords..., c_part.element_stride);
        } else if constexpr (sizeof...(Coords) == 3) {
            child_flat_local_idx = strategy.get_index_3d(p_child_coords..., c_part.element_stride);
        } else if constexpr (sizeof...(Coords) == 4) {
            child_flat_local_idx = strategy.get_index_4d(p_child_coords..., c_part.element_stride);
        }

        int64_t global_child_flat_idx = static_cast<int64_t>(actual_parent_idx * bridge_info.child_cells_per_parent) + child_flat_local_idx;

        #ifdef NDEBUG
            [[assume(c_selection.is_selected(global_child_flat_idx))]];
        #else
            assert(c_selection.is_selected(global_child_flat_idx) && "Bridge child access outside child selection mask!");
        #endif

        if constexpr (sizeof...(Coords) == 2) {
            return *strategy.resolve_2d(reinterpret_cast<TChild*>(child_block_ptr), p_child_coords..., c_part.element_stride);
        } else if constexpr (sizeof...(Coords) == 3) {
            return *strategy.resolve_3d(reinterpret_cast<TChild*>(child_block_ptr), p_child_coords..., c_part.element_stride);
        } else if constexpr (sizeof...(Coords) == 4) {
            return *strategy.resolve_4d(reinterpret_cast<TChild*>(child_block_ptr), p_child_coords..., c_part.element_stride);
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BRIDGE_VIEW_H