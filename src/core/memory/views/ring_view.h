#ifndef IDEAM_CORE_MEMORY_RING_VIEW_H
#define IDEAM_CORE_MEMORY_RING_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * RingView<T, Strategy>
 * Specialized for circular buffer streaming with optional spatial addressing.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Selection logic is applied to the physical buffer index after the read-head offset is calculated.
 */
template<typename T, typename Strategy = FlatStrategy>
struct RingView {
    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;
    uint32_t* read_index_ptr = nullptr;
    uint32_t* write_index_ptr = nullptr;

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
     * at
     * Resolves coordinates relative to the current READ head.
     * Guarded by Selection check against the resulting physical index.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& at(Args... p_coords) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;
        
        int64_t logical_idx = 0;
        if constexpr (sizeof...(Args) == 1) {
            logical_idx = static_cast<int64_t>(p_coords...);
        } else if constexpr (sizeof...(Args) == 2) {
            logical_idx = strategy.get_index_2d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 3) {
            logical_idx = strategy.get_index_3d(p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 4) {
            logical_idx = strategy.get_index_4d(p_coords..., part.element_stride);
        }

        const size_t physical_idx = (*read_index_ptr + static_cast<size_t>(logical_idx)) % part.element_count;

        if (!selection.is_selected(static_cast<int64_t>(physical_idx))) {
            return *head_ptr;
        }

        return *strategy.resolve(head_ptr, physical_idx, part.element_stride, part.capacity_bytes);
    }

    /**
     * push
     * Writes to the write-head and advances. Returns false if full.
     */
    inline bool push(const T& p_value) {
        const auto& part = grant->parts[grant_part_index];
        uint32_t next_write = (*write_index_ptr + 1) % static_cast<uint32_t>(part.element_count);

        if (next_write == *read_index_ptr) return false; 

        // Grant check: Ensure the physical write head is within the allowed selection
        if (!part.selection.is_selected(static_cast<int64_t>(*write_index_ptr))) return false;

        T* target = strategy.resolve(head_ptr, *write_index_ptr, part.element_stride, part.capacity_bytes);
        *target = p_value;
        *write_index_ptr = next_write;
        return true;
    }

    /**
     * pop
     * Reads from the read-head and advances. Returns false if empty.
     */
    inline bool pop(T& r_out_value) {
        if (*read_index_ptr == *write_index_ptr) return false; 

        const auto& part = grant->parts[grant_part_index];
        
        // Grant check: Ensure the physical read head is within the allowed selection
        if (!part.selection.is_selected(static_cast<int64_t>(*read_index_ptr))) return false;

        T* source = strategy.resolve(head_ptr, *read_index_ptr, part.element_stride, part.capacity_bytes);
        r_out_value = *source;
        *read_index_ptr = (*read_index_ptr + 1) % static_cast<uint32_t>(part.element_count);
        return true;
    }

    /**
     * operator[]
     * Selection-Relative Linear Access.
     * Maps p_selection_index to the buffer index via MemoryBufferSelectionPOD,
     * then applies the ring-buffer read-head offset.
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

        size_t selected_buffer_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE:
                selected_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                break;
            case SelectionMode::DENSE:
                selected_buffer_index = p_selection_index;
                break;
            case SelectionMode::RANGE:
                selected_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                break;
        }

        // Apply read-head offset for relative linear iteration
        const size_t physical_idx = (*read_index_ptr + selected_buffer_index) % part.element_count;

        // Final safety check: ensure the resulting ring-relative index is actually selected
        if (!selection.is_selected(static_cast<int64_t>(physical_idx))) {
            return *head_ptr;
        }

        return *strategy.resolve(head_ptr, physical_idx, part.element_stride, part.capacity_bytes);
    }

    [[nodiscard]] inline size_t available() const {
        const auto& part = grant->parts[grant_part_index];
        if (*write_index_ptr >= *read_index_ptr) {
            return *write_index_ptr - *read_index_ptr;
        }
        return part.element_count - (*read_index_ptr - *write_index_ptr);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_RING_VIEW_H