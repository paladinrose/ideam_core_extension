#ifndef IDEAM_CORE_BRIDGE_VIEW_H
#define IDEAM_CORE_BRIDGE_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <array>
#include <cstdint>
#include <type_traits>

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
 * Iteration (operator[]) drives through the Parent Selection.
 */
template<typename TParent, typename TChild, size_t DimCount, typename Strategy = Spatial2DStrategy>
struct BridgeView {
    // --- 8-Byte Block ---
    TParent* parent_head = nullptr;
    TChild* child_head = nullptr;
    const MemoryGrantPOD* parent_grant = nullptr;
    const MemoryGrantPOD* child_grant = nullptr;

    // --- 4-Byte Block ---
    uint32_t parent_part_idx = 0;
    uint32_t child_part_idx = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Policy & Metadata ---
    [[no_unique_address]] Strategy strategy;
    BridgeMetadata<DimCount> bridge_info;

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::RANDOM_ACCESS | 
        ViewCapability::COMPOSITE_VIEW |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr bool is_spatial = Strategy::is_spatial;
    static constexpr bool is_simd = false;

    /**
     * is_valid
     * Validates both grants and their respective versions.
     */
    [[nodiscard]] inline bool is_valid() const {
        if (!parent_grant || !child_grant || !parent_grant->active || !child_grant->active) return false;
        
        const auto& p_part = parent_grant->parts[parent_part_idx];
        const auto& c_part = child_grant->parts[child_part_idx];

        return (p_part.buffer_version_at_issue == baked_buffer_version &&
                c_part.buffer_version_at_issue == baked_buffer_version);
    }

    /**
     * compose_child_selection
     * Generates a new SelectionPOD for the child buffer based on the parent's current selection.
     * Note: This only supports RANGE and DENSE parents for O(1) composition. 
     * SPARSE parents require a heap allocation for the new index list (handled by BufferManager).
     */
    [[nodiscard]] inline MemoryBufferSelectionPOD compose_child_selection() const {
        const auto& p_sel = parent_grant->parts[parent_part_idx].selection;
        MemoryBufferSelectionPOD c_sel;

        c_sel.mode = p_sel.mode;
        c_sel.element_count = p_sel.element_count * bridge_info.child_cells_per_parent;
        
        if (p_sel.mode == SelectionMode::RANGE) {
            c_sel.start_index = p_sel.start_index * bridge_info.child_cells_per_parent;
        } else if (p_sel.mode == SelectionMode::DENSE) {
            // Bitmask scaling: This is technically a "block-mask". 
            // We reuse the parent mask; the consumer must understand that 1 bit = 1 parent block.
            c_sel.data.bitmask = p_sel.data.bitmask;
        } else if (p_sel.mode == SelectionMode::SPARSE) {
            // Logic Error: BridgeView cannot self-allocate a new sparse index list.
            // This must be requested via the MemoryManager to ensure tracking.
            c_sel.mode = SelectionMode::RANGE;
            c_sel.start_index = 0;
            c_sel.element_count = 0; 
        }

        return c_sel;
    }

    /**
     * operator[]
     * Linear Access to the Parent element via Parent Selection.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline TParent& operator[](size_t p_selection_index) const {
        const auto& part = parent_grant->parts[parent_part_idx];
        const auto& selection = part.selection;

        if (p_selection_index >= static_cast<size_t>(selection.element_count)) {
            return *parent_head;
        }

        size_t actual_idx = selection.get_absolute_index(p_selection_index);
        return *strategy.resolve(parent_head, actual_idx, part.element_stride, part.capacity_bytes);
    }

    /**
     * get_child_base
     * Returns the raw base pointer for the child block associated with a parent index.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline uint8_t* get_child_base(size_t p_parent_selection_idx) const {
        const auto& p_part = parent_grant->parts[parent_part_idx];
        const auto& c_part = child_grant->parts[child_part_idx];
        
        size_t abs_parent_idx = p_part.selection.get_absolute_index(p_parent_selection_idx);
        
        const size_t block_byte_offset = abs_parent_idx * bridge_info.child_cells_per_parent * c_part.element_stride;
        return reinterpret_cast<uint8_t*>(child_head) + block_byte_offset;
    }

    /**
     * at_child
     * Access a specific child element within a parent's block using spatial coordinates.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline TChild& at_child(size_t p_parent_selection_idx, Args... p_coords) const {
        static_assert(sizeof...(Args) == DimCount, "Coord count mismatch.");
        
        uint8_t* child_block_ptr = get_child_base(p_parent_selection_idx);
        const auto& c_part = child_grant->parts[child_part_idx];

        TChild* resolved = nullptr;
        if constexpr (sizeof...(Args) == 2) {
            resolved = strategy.resolve_2d(reinterpret_cast<TChild*>(child_block_ptr), p_coords..., c_part.element_stride);
        } else if constexpr (sizeof...(Args) == 3) {
            resolved = strategy.resolve_3d(reinterpret_cast<TChild*>(child_block_ptr), p_coords..., c_part.element_stride);
        }
        
        return *resolved;
    }

    /**
     * size
     * Returns the count of selected Parent elements.
     */
    [[nodiscard]] inline size_t size() const {
        return static_cast<size_t>(parent_grant->parts[parent_part_idx].selection.element_count);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BRIDGE_VIEW_H