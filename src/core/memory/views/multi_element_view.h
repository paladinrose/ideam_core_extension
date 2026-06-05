#pragma once

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace ideam::core {

/**
 * MultiElementView<Strategy>
 * A specialized view for high-performance multi-member access within a single buffer.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Facilitates "plucking" multiple members from an element base via raw offsets.
 * * [C++26 Enabled]: Multidimensional Subscripts and [[assume]] attributes.
 */
template<IsMemoryStrategy Strategy = AoSStrategy>
struct MultiElementView {
    // --- 8-Byte Block (16 Bytes) ---
    uint8_t* head_ptr = nullptr; 
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
    // Locks the base members to exactly 32 bytes (half a cache line).
    uint8_t reserved_padding[4] = {0};


    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::RANDOM_ACCESS | 
        ViewCapability::MULTI_COMPONENT_ACCESS |
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr BufferLayoutType supported_layouts = 
        BufferLayoutType::FLAT | BufferLayoutType::AOS;

    static constexpr ViewStrategies supported_strategies = 
        ViewStrategies::ANY_LINEAR | ViewStrategies::ANY_SPATIAL;

    // View strictly handles byte-level structural routing, delegating type safety to pluck<T>.
    static constexpr DataType supported_types = DataType::ANY;

    static constexpr uint32_t lane_width = 1;

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
     * Binds the raw memory block to the View's byte-level pointer.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void bind(const GrantPartPOD* p_part) noexcept {
        head_ptr = reinterpret_cast<uint8_t*>(p_part->raw_base_ptr);
    }
    
    /**
     * operator[] (C++23/26 Multidimensional Subscript)
     * Replaces get_base(). Resolves spatial coordinates down to the raw uint8_t* byte base.
     */
    template<typename... Coords>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline uint8_t* operator[](Coords... p_coords) const noexcept {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        // --- 1D LINEAR ACCESS PATH ---
        if constexpr (sizeof...(Coords) == 1 && !Strategy::is_spatial) {
            size_t p_selection_index = static_cast<size_t>((p_coords, ...));
            
            #ifdef NDEBUG
                [[assume(p_selection_index < static_cast<size_t>(selection.element_count))]];
            #else
                assert(p_selection_index < static_cast<size_t>(selection.element_count) && "MultiElementView out of bounds!");
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
                return Strategy::template resolve<uint8_t>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
            } else {
                return strategy.template resolve<uint8_t>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
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

            // C++26 Optimizer Hinting: Forces the compiler to trust the memory boundaries
            #ifdef NDEBUG
                [[assume(selection.is_selected(flat_idx))]];
            #else
                assert(selection.is_selected(flat_idx) && "Spatial multi-element access outside selection mask!");
            #endif

            if constexpr (sizeof...(Coords) == 2) {
                return strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 3) {
                return strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
            } else if constexpr (sizeof...(Coords) == 4) {
                return strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
            }
        } 
        else {
            static_assert(sizeof...(Coords) < 0, "Invalid coordinate dimensions provided for the assigned View Strategy!");
        }
    }

    /**
     * pluck<T>
     * Helper to retrieve a specific member from an element base via byte offset.
     * Guaranteed branchless and zero-cost in Release builds.
     */
    template<typename T>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T& pluck(uint8_t* p_element_base, size_t p_byte_offset) noexcept {
        #ifdef NDEBUG
            [[assume(p_element_base != nullptr)]];
        #else
            assert(p_element_base != nullptr && "Attempted to pluck from a null base pointer!");
        #endif

        return *reinterpret_cast<T*>(p_element_base + p_byte_offset);
    }
};

#ifndef __INTELLISENSE__
static_assert(sizeof(MultiElementView<AoSStrategy>) == 32, "MultiElementView base layout alignment failed!");
#endif

template<IsMemoryStrategy Strategy>
struct ViewTraits<MultiElementView<Strategy>> {
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::SPATIAL_ACCESS | 
        ViewCapability::MULTI_COMPONENT_ACCESS;
        
    static constexpr BufferLayoutType supported_layouts = 
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
        
    static constexpr ViewStrategies supported_strategies = ViewStrategies::ANY;
    static constexpr DataType       supported_types      = DataType::ANY;
    static constexpr uint32_t       lane_width           = 1;

    // Spatial Contracts
    static constexpr bool is_static_stencil = false; 
    static constexpr size_t kernel_size     = 0;
    static constexpr size_t dimensions      = Strategy::dimensions;
};

} // namespace ideam::core

 // IDEAM_CORE_MULTI_ELEMENT_VIEW_H