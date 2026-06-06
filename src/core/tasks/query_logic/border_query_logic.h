#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <algorithm>
#include <vector>

namespace ideam::core {

/**
 * BorderQueryLogic
 * Identifies "border" elements: those that are either physically on the edge 
 * of the grid or have at least one neighbor missing from the current selection.
 * CULL: Strips the halo (keeps strictly interior elements).
 * ADD: Spatially dilates the selection by queueing unselected neighbors.
 */
template <typename T, typename T_Strategy>
struct BorderQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StencilView<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = T_Strategy::dimensions; 
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;
    
    // UI/Compiler Routing
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;

    static godot::Array get_ui_properties() {
        return godot::Array();
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept { }
    
    template <QueryOp Op, typename T_View, typename T_StrategyType>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (r_selection.mode != SelectionMode::SPARSE) return;

        if constexpr (Op == QueryOp::CULL) {
            _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_dilation(r_selection, p_view, p_context);
        }
    }

private:
    template <typename T_View>
    int64_t _get_spatial_neighbor_index(const T_View& p_view, int64_t p_current_idx, uint32_t p_dim, int64_t p_step) const noexcept {
        const auto& part = p_view.grant->parts[p_view.grant_part_index];
        const size_t stride = part.element_stride;
        const int64_t total_elements = static_cast<int64_t>(part.capacity_bytes / stride);

        if constexpr (T_Strategy::dimensions == 1) {
            int64_t x = p_current_idx + p_step;
            if (x >= 0 && x < total_elements) return x;
        } 
        else if constexpr (T_Strategy::dimensions == 2) {
            int64_t width = p_view.strategy.stride_y / stride;
            int64_t x = p_current_idx % width;
            int64_t y = p_current_idx / width;
            
            if (p_dim == 0) x += p_step;
            else if (p_dim == 1) y += p_step;
            
            if (x >= 0 && x < width && y >= 0 && y < (total_elements / width)) {
                return x + (y * width);
            }
        }
        else if constexpr (T_Strategy::dimensions == 3) {
            int64_t width = p_view.strategy.stride_y / stride;
            int64_t height = p_view.strategy.stride_z / p_view.strategy.stride_y;
            int64_t x = p_current_idx % width;
            int64_t y = (p_current_idx / width) % height;
            int64_t z = p_current_idx / (width * height);
            
            if (p_dim == 0) x += p_step;
            else if (p_dim == 1) y += p_step;
            else if (p_dim == 2) z += p_step;
            
            if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < (total_elements / (width * height))) {
                return x + (y * width) + (z * width * height);
            }
        }
        else if constexpr (T_Strategy::dimensions == 4) {
            int64_t width = p_view.strategy.stride_y / stride;
            int64_t height = p_view.strategy.stride_z / p_view.strategy.stride_y;
            int64_t depth = p_view.strategy.stride_w / p_view.strategy.stride_z;
            int64_t x = p_current_idx % width;
            int64_t y = (p_current_idx / width) % height;
            int64_t z = (p_current_idx / (width * height)) % depth;
            int64_t w = p_current_idx / (width * height * depth);
            
            if (p_dim == 0) x += p_step;
            else if (p_dim == 1) y += p_step;
            else if (p_dim == 2) z += p_step;
            else if (p_dim == 3) w += p_step;
            
            if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth && w >= 0) {
                return x + (y * width) + (z * width * height) + (w * width * height * depth);
            }
        }

        return -1; // Out of bounds
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        std::vector<int64_t> sorted_indices(r_selection.data.indices, r_selection.data.indices + r_selection.element_count);
        std::sort(sorted_indices.begin(), sorted_indices.end());

        int64_t write_ptr = 0;
        const int64_t original_count = r_selection.element_count;

        for (int64_t i = 0; i < original_count; ++i) {
            const int64_t current_idx = r_selection.data.indices[i];
            bool is_border = false;

            for (uint32_t d = 0; d < T_Strategy::dimensions; ++d) {
                for (int64_t step : {-1LL, 1LL}) {
                    int64_t neighbor_idx = _get_spatial_neighbor_index(p_view, current_idx, d, step);

                    if (neighbor_idx == -1 || !std::binary_search(sorted_indices.begin(), sorted_indices.end(), neighbor_idx)) {
                        is_border = true;
                        break;
                    }
                }
                if (is_border) break;
            }

            // Halo Stripping: We only retain elements that are NOT borders.
            if (!is_border) {
                r_selection.data.indices[write_ptr++] = current_idx;
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_dilation(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        std::vector<int64_t> sorted_indices(r_selection.data.indices, r_selection.data.indices + r_selection.element_count);
        std::sort(sorted_indices.begin(), sorted_indices.end());

        const int64_t original_count = r_selection.element_count;
        const uint64_t* unclaimed = r_selection.unclaimed_mask;

        for (int64_t i = 0; i < original_count; ++i) {
            const int64_t current_idx = r_selection.data.indices[i];

            for (uint32_t d = 0; d < T_Strategy::dimensions; ++d) {
                for (int64_t step : {-1LL, 1LL}) {
                    int64_t neighbor_idx = _get_spatial_neighbor_index(p_view, current_idx, d, step);

                    // If neighbor index exists but isn't part of our selection, it's a dilation candidate
                    if (neighbor_idx != -1 && !std::binary_search(sorted_indices.begin(), sorted_indices.end(), neighbor_idx)) {
                        
                        // Check if the neighbor is globally unclaimed/available before queuing
                        if (unclaimed && (unclaimed[neighbor_idx >> 6] & (1ULL << (neighbor_idx & 63)))) {
                            p_ctx.queue_selection_command(target_buffer_id, neighbor_idx);
                        }
                    }
                }
            }
        }
    }
};

} // namespace ideam::core