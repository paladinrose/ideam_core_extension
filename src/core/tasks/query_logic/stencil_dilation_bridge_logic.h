#ifndef IDEAM_CORE_STENCIL_DILATION_BRIDGE_LOGIC_H
#define IDEAM_CORE_STENCIL_DILATION_BRIDGE_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <vector>

namespace ideam::core {

/**
 * StencilDilationBridgeLogic<T_Coord, T_Strategy, DimCount>
 * Target: Grid (ANY_SPATIAL). Source: Entities (SPARSE_SET).
 * Maps Entity coordinates to a Grid, then applies a Stencil View to activate all surrounding cells.
 * REQUIREMENT: T_View MUST be bound to the Source Buffer.
 */
template <typename T_Coord, typename T_Strategy, uint32_t DimCount>
struct StencilDilationBridgeLogic {
    using ValueType       = T_Coord; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T_Coord, T_Strategy, DimCount>;

    static constexpr LogicRequirement requirements = LogicRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;

    static constexpr bool supports_cull = false; // Stencils are used for dilation/addition
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* source_selection = nullptr; 
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_View_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_source_view) const {
        
        if (!source_selection || Op != QueryOp::ADD) return;

        // 1. Build the dilated projection mask locally
        std::vector<uint64_t> dilation_mask((r_selection.capacity + 63) >> 6, 0ULL);
        const T_Strategy& strategy = p_source_view.get_strategy();

        auto apply_stencil_to_mask = [&](int64_t src_idx) {
            T_Coord center_coord = p_source_view[src_idx];
            int64_t center_cell = strategy.get_cell_index(center_coord);
            
            if (center_cell >= 0 && center_cell < r_selection.capacity) {
                // Apply the center cell
                dilation_mask[center_cell >> 6] |= (1ULL << (center_cell & 63));
                
                // Fetch stencil offsets via the View (e.g., 8 surrounding cells in 2D)
                const auto& offsets = p_source_view.get_stencil_offsets();
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
        const int64_t words = (r_selection.capacity + 63) >> 6;
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

    template<typename T_View, typename T_View_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }
};

} // namespace ideam::core

#endif // IDEAM_CORE_STENCIL_DILATION_BRIDGE_LOGIC_H