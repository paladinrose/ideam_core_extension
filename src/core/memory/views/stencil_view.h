#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>
#include <tuple>
#include <cassert>

namespace ideam::core {

/**
 * StencilView<T, Strategy, DimCount>
 * Provides dynamic, relative neighbor access with zero-cost coordinate folding up to 4D.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * * [C++26 Enabled]: Multidimensional Subscripts, [[assume]] optimizations, and `mutable` cursors.
 */
template<typename T, IsMemoryStrategy Strategy>
struct StencilView {
    
    static constexpr size_t DimCount = Strategy::dimensions;

    // --- 8-Byte Block (24 Bytes) ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;
    
    // Mutable allows the cursor to move dynamically without violating const-correctness.
    mutable uint8_t* center_ptr = nullptr;

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

    // --- Explicit Alignment Padding (12 Bytes) ---
    // Locks the base members to exactly 48 bytes (perfect 16-byte multiple).
    uint8_t reserved_padding[12] = {0};

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::SPATIAL_ACCESS |
        ViewCapability::STENCIL_ACCESS;

    static constexpr BufferLayoutType supported_layouts = 
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA | 
        BufferLayoutType::SPARSE_SET | BufferLayoutType::TILED_SOA;

    // Geometric coordinate folding relies exclusively on multi-dimensional strides.
    static constexpr ViewStrategies supported_strategies = ViewStrategies::ANY_SPATIAL;

    // Pointer-math based; agnostic to the underlying memory payload.
    static constexpr DataType supported_types = DataType::ANY;

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
     * Focuses the Stencil cursor at the specified spatial/linear coordinates.
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

        T* resolved = nullptr;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "StencilView out of bounds!");
            #endif

            size_t actual_buffer_index = 0;
            switch (selection.mode) {
                case SelectionMode::SPARSE:
                    actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                    break;
                case SelectionMode::DENSE:
                    #ifdef NDEBUG
                        [[assume(selection.is_selected(static_cast<int64_t>(p_selection_index)))]];
                    #else
                        assert(selection.is_selected(static_cast<int64_t>(p_selection_index)) && "Accessed unselected DENSE index!");
                    #endif
                    actual_buffer_index = p_selection_index;
                    break;
                case SelectionMode::RANGE:
                    actual_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                    break;
            }

            if constexpr (std::is_empty_v<Strategy>) {
                resolved = Strategy::template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            } else {
                resolved = strategy.template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            }
        } 
        // --- N-DIMENSIONAL SPATIAL ACCESS PATH ---
        else if constexpr (sizeof...(Coords) > 1 && Strategy::is_spatial) {
            int64_t flat_idx = 0;
            
            if constexpr (sizeof...(Coords) == 2) {
                flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
                resolved = strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
                resolved = strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
                resolved = strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            }

            #ifdef NDEBUG
                [[assume(selection.is_selected(flat_idx))]];
            #else
                assert(selection.is_selected(flat_idx) && "Spatial Stencil focus outside selection mask!");
            #endif
        } 
        else {
            static_assert(false, "Invalid coordinate dimensions for Stencil Strategy!");
        }

        // Update the mutable internal cursor legally
        center_ptr = reinterpret_cast<uint8_t*>(resolved);
        return *resolved;
    }

    /**
     * center
     * Returns the currently focused element.
     */
    [[nodiscard]] inline T& center() const noexcept {
        #ifndef NDEBUG
            assert(center_ptr != nullptr && "StencilView: Attempted to read center before focusing via operator[]!");
        #endif
        return *reinterpret_cast<T*>(center_ptr);
    }

    /**
     * neighbor (Dynamic Spatial Offsets)
     * Extremely fast relative addressing. Fetches a neighbor by applying spatial coordinate
     * offsets (dx, dy, dz) directly to the cached center pointer via Strategy Strides.
     */
    template<typename... Offsets>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& neighbor(Offsets... p_offsets) const noexcept {
        static_assert(sizeof...(Offsets) == DimCount, "Neighbor offset dimensions must match StencilView DimCount!");
        
        #ifndef NDEBUG
            assert(center_ptr != nullptr && "StencilView: Attempted to fetch neighbor before focusing!");
        #endif

        const auto& part = grant->parts[grant_part_index];
        intptr_t byte_offset = 0;

        // Compile-time folding of the geometric strides
        if constexpr (sizeof...(Offsets) == 1) {
            auto [dx] = std::tuple{static_cast<intptr_t>(p_offsets)...};
            byte_offset = dx * part.element_stride;
        } 
        else if constexpr (sizeof...(Offsets) == 2) {
            auto [dx, dy] = std::tuple{static_cast<intptr_t>(p_offsets)...};
            byte_offset = (dx * part.element_stride) + (dy * strategy.stride_y);
        } 
        else if constexpr (sizeof...(Offsets) == 3) {
            auto [dx, dy, dz] = std::tuple{static_cast<intptr_t>(p_offsets)...};
            byte_offset = (dx * part.element_stride) + (dy * strategy.stride_y) + (dz * strategy.stride_z);
        } 
        else if constexpr (sizeof...(Offsets) == 4) {
            auto [dx, dy, dz, dw] = std::tuple{static_cast<intptr_t>(p_offsets)...};
            byte_offset = (dx * part.element_stride) + (dy * strategy.stride_y) + (dz * strategy.stride_z) + (dw * strategy.stride_w);
        }

        return *reinterpret_cast<T*>(center_ptr + byte_offset);
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(StencilView<int, FlatStrategy>) == 48, "StencilView base layout alignment failed!");
#endif

template<typename T, IsMemoryStrategy Strategy>
struct ViewTraits<StencilView<T, Strategy>> {
    static constexpr ViewCapability capabilities = StencilView<T, Strategy>::capabilities;
        
    static constexpr BufferLayoutType supported_layouts = StencilView<T, Strategy>::supported_layouts;
        
    static constexpr ViewStrategies supported_strategies = StencilView<T, Strategy>::supported_strategies;
    static constexpr DataType       supported_types      = StencilView<T, Strategy>::supported_types;
    static constexpr uint32_t       lane_width           = StencilView<T, Strategy>::lane_width;

    // --- Extracted Spatial & Kernel Contracts ---
    static constexpr bool is_static_stencil = false;
    static constexpr size_t kernel_size     = 0;
    static constexpr size_t dimensions      = Strategy::dimensions; 
};

} // namespace ideam::core

 // IDEAM_CORE_STENCIL_VIEW_H