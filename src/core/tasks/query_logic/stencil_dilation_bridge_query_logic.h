#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstring>
#include <bit>

namespace ideam::core {

/**
 * StencilDilationBridgeQueryLogic<T_Coord, T_Strategy, DimCount>
 * Target: Grid (ANY_SPATIAL). Source: Entities (SPARSE_SET).
 * Maps Entity coordinates to a Grid, then applies a Stencil View to activate all surrounding cells.
 * REQUIREMENT: T_View MUST be bound to the Source Buffer.
 * TRANSIENT MEMORY: Requires `((capacity + 63) / 64) * 8` bytes of workspace.
 */
template <typename T_Coord, typename T_Strategy, uint32_t DimCount>
struct StencilDilationBridgeQueryLogic {
    using ValueType       = T_Coord; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T_Coord, T_Strategy, DimCount>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::STENCIL_ACCESS | ViewCapability::SPATIAL_ACCESS | ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::VECTOR4I | DataType::VECTOR4D;
    static constexpr size_t transient_workspace_bytes     = 0; // Dynamically allocated by Job Graph

    static constexpr bool supports_cull = false; // Stencils are used for dilation/addition
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* source_selection = nullptr; 
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy_Inner>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!source_selection || Op != QueryOp::ADD || !p_context.local_workspace) return;

        // 1. Build the dilated projection mask locally in Transient Memory
        const int64_t words = (r_selection.capacity + 63) >> 6;
        const size_t bytes_needed = words * sizeof(uint64_t);
        uint64_t* dilation_mask = static_cast<uint64_t*>(p_context.local_workspace);
        std::memset(dilation_mask, 0, bytes_needed);

        const T_Strategy& strategy = p_view.get_strategy();

        auto apply_stencil_to_mask = [&](int64_t src_idx) {
            // CORRECTED: Safe extraction
            T_Coord center_coord = _read_view(p_view, src_idx);
            int64_t center_cell = strategy.get_cell_index(center_coord);
            
            if (center_cell >= 0 && center_cell < r_selection.capacity) {
                // Apply the center cell
                dilation_mask[center_cell >> 6] |= (1ULL << (center_cell & 63));
                
                // Fetch stencil offsets via the View (e.g., 8 surrounding cells in 2D)
                const auto& offsets = p_view.get_stencil_offsets();
                for (int32_t offset : offsets) {
                    int64_t neighbor_cell = center_cell + offset;
                    if (neighbor_cell >= 0 && neighbor_cell < r_selection.capacity) {
                        dilation_mask[neighbor_cell >> 6] |= (1ULL << (neighbor_cell & 63));
                    }
                }
            }
        };

        if (source_selection->mode == SelectionMode::SPARSE) {
            for (int64_t i = 0; i < source_selection->element_count; ++i) {
                apply_stencil_to_mask(source_selection->data.indices[i]);
            }
        } else if (source_selection->mode == SelectionMode::DENSE) {
            const uint64_t* src_bitset = source_selection->data.bitset;
            for (int64_t i = 0; i < source_selection->capacity; ++i) {
                if (src_bitset[i >> 6] & (1ULL << (i & 63))) apply_stencil_to_mask(i);
            }
        }

        // 2. Addition: Queue target cells that are in the dilated mask AND globally unclaimed
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = dilation_mask[w];
            if (unclaimed) mask &= unclaimed[w];
            
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                p_context.queue_selection_command(target_buffer_id, global_index);
                mask &= (mask - 1); 
            }
        }
    }

private:
    // --- The DOD View Adapter ---
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T_Coord _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T_Coord*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T_Coord>(p_view[idx]); }) {
            return static_cast<T_Coord>(p_view[idx]);
        } else {
            return T_Coord{}; 
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_STENCIL_DILATION_BRIDGE_QUERY_LOGIC_H