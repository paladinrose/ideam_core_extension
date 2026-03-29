#ifndef IDEAM_CORE_SWAP_VIEW_H
#define IDEAM_CORE_SWAP_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <utility>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * SwapElementProxy<T>
 * A zero-overhead C++26 abstraction. Routes reads to State T and writes to State T+1.
 * Completely optimized out into raw registers during Release builds.
 */
template<typename T>
struct SwapElementProxy {
    const T& current_state;
    T& next_state;

    // Implicitly cast to the Read Buffer when used in an expression
    [[nodiscard]] inline operator const T&() const noexcept { return current_state; }
    
    // Route assignments directly into the Write Buffer
    inline SwapElementProxy& operator=(const T& p_value) noexcept { next_state = p_value; return *this; }
    inline SwapElementProxy& operator=(T&& p_value) noexcept { next_state = std::move(p_value); return *this; }
    
    // Explicit escape hatches for complex template deductions
    [[nodiscard]] inline const T& read() const noexcept { return current_state; }
    inline T& write() noexcept { return next_state; }
};

/**
 * SwapView<T, Strategy>
 * A dual-buffer view for ping-pong simulation states (Double Buffering).
 * Selection-aware: Iteration (operator[]) follows the Read-buffer's selection.
 * * [C++26 Enabled]: Multidimensional Subscripts, [[assume]] optimizations, and Proxy Returns.
 */
template<typename T, IsMemoryStrategy Strategy = FlatStrategy>
struct SwapView {
    // --- 8-Byte Aligned Block (32 Bytes) ---
    T* read_head = nullptr;
    T* write_head = nullptr;
    const MemoryGrantPOD* read_grant = nullptr;
    const MemoryGrantPOD* write_grant = nullptr;

    // --- 4-Byte Aligned Block (16 Bytes) ---
    uint32_t read_part_idx = 0;
    uint32_t write_part_idx = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Explicit Alignment Padding (16 Bytes) ---
    // Locks the base members to exactly 64 bytes (1 perfect hardware cache line).
    uint8_t reserved_padding[16] = {0};

    // --- Policy ---
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
     * Validates both Read and Write states against the global authority.
     */
    [[nodiscard]] inline bool is_valid() const noexcept {
        if (!read_grant || !read_grant->active || !write_grant || !write_grant->active) return false;
        
        const auto& r_part = read_grant->parts[read_part_idx];
        const auto& w_part = write_grant->parts[write_part_idx];

        if (r_part.buffer_version_at_issue != baked_buffer_version || 
            w_part.buffer_version_at_issue != baked_buffer_version) return false;

        if (read_grant->global_manager_version_ptr && 
            *read_grant->global_manager_version_ptr != baked_manager_version) return false;

        return r_part.selection.is_valid() && w_part.selection.is_valid();
    }

    /**
     * operator[] (C++23/26 Multidimensional Subscript)
     * Replaces both read_at and write_at. Returns a zero-overhead SwapElementProxy.
     */
    template<typename... Coords>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline SwapElementProxy<T> operator[](Coords... p_coords) const noexcept {
        const auto& r_part = read_grant->parts[read_part_idx];
        const auto& w_part = write_grant->parts[write_part_idx];
        const auto& r_selection = r_part.selection;

        T* r_resolved = nullptr;
        T* w_resolved = nullptr;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords)...);
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(r_selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(r_selection.element_count) && "SwapView linear access out of bounds!");
            #endif

            size_t actual_buffer_index = 0;
            switch (r_selection.mode) {
                case SelectionMode::SPARSE:
                    actual_buffer_index = static_cast<size_t>(r_selection.data.indices[p_selection_index]);
                    break;
                case SelectionMode::DENSE:
                    #ifdef NDEBUG
                        [[assume(r_selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(r_selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
                    actual_buffer_index = p_selection_index;
                    break;
                case SelectionMode::RANGE:
                    actual_buffer_index = static_cast<size_t>(r_selection.start_index) + p_selection_index;
                    break;
            }

            if constexpr (std::is_empty_v<Strategy>) {
                r_resolved = Strategy::template resolve<T>(read_head, actual_buffer_index, r_part.element_stride, r_part.capacity_bytes);
                w_resolved = Strategy::template resolve<T>(write_head, actual_buffer_index, w_part.element_stride, w_part.capacity_bytes);
            } else {
                r_resolved = strategy.template resolve<T>(read_head, actual_buffer_index, r_part.element_stride, r_part.capacity_bytes);
                w_resolved = strategy.template resolve<T>(write_head, actual_buffer_index, w_part.element_stride, w_part.capacity_bytes);
            }
        } 
        // --- N-DIMENSIONAL SPATIAL ACCESS PATH ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            int64_t flat_idx = 0;
            
            if constexpr (sizeof...(Coords) == 2) {
                flat_idx = strategy.get_index_2d(p_coords..., r_part.element_stride);
                r_resolved = strategy.resolve_2d(read_head, p_coords..., r_part.element_stride);
                w_resolved = strategy.resolve_2d(write_head, p_coords..., w_part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                flat_idx = strategy.get_index_3d(p_coords..., r_part.element_stride);
                r_resolved = strategy.resolve_3d(read_head, p_coords..., r_part.element_stride);
                w_resolved = strategy.resolve_3d(write_head, p_coords..., w_part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                flat_idx = strategy.get_index_4d(p_coords..., r_part.element_stride);
                r_resolved = strategy.resolve_4d(read_head, p_coords..., r_part.element_stride);
                w_resolved = strategy.resolve_4d(write_head, p_coords..., w_part.element_stride);
            }

            #ifdef NDEBUG
                [[assume(r_selection.is_selected(flat_idx))]];
            #else
                assert(r_selection.is_selected(flat_idx) && "Spatial access outside Ping-Pong selection mask!");
            #endif
        } 
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions provided for SwapView Strategy!");
        }

        return { *r_resolved, *w_resolved };
    }

    /**
     * swap_buffers
     * Swaps the roles of the internal pointers and grants at the end of a simulation tick.
     * (Thread-safe to call during the graph barrier sync phase).
     */
    inline void swap_buffers() noexcept {
        std::swap(read_head, write_head);
        std::swap(read_grant, write_grant);
        std::swap(read_part_idx, write_part_idx);
    }
};

static_assert(sizeof(SwapView<int, FlatStrategy>) == 64, "SwapView base layout alignment failed!");

} // namespace ideam::core

#endif // IDEAM_CORE_SWAP_VIEW_H