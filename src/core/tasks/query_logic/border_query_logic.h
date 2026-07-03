#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

namespace ideam::core {

/**
 * BorderQueryLogic
 * Identifies "border" elements: those that are either physically on the edge 
 * of the grid or have at least one neighbor missing from the current selection.
 * CULL: Strips the halo (keeps strictly interior elements).
 * ADD: Spatially dilates the selection by queueing unselected neighbors.
 * TRANSIENT MEMORY: Requires `capacity * 1` bytes for O(1) selection mapping.
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

    static constexpr size_t transient_workspace_bytes     = 0; // User must set via Graph to `capacity * 1`
    
    // UI/Compiler Routing
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr std::string_view display_name = "Border";
    
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
        
        if (r_selection.mode != SelectionMode::SPARSE || r_selection.element_count == 0 || !p_context.local_workspace) return;

        if constexpr (Op == QueryOp::CULL) {
            _cull_sparse(r_selection, p_view, p_context);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_dilation(r_selection, p_view, p_context);
        }
    }

private:
    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        const int64_t total_capacity = r_selection.capacity;
        const auto& part = p_view.grant->parts[p_view.grant_part_index];
        const size_t stride = part.element_stride;

        // 1. O(1) Transient Lookup Map Allocation
        uint8_t* is_selected = static_cast<uint8_t*>(p_context.local_workspace);
        for (int64_t i = 0; i < total_capacity; ++i) is_selected[i] = 0;
        
        const int64_t original_count = r_selection.element_count;
        for (int64_t i = 0; i < original_count; ++i) {
            is_selected[r_selection.data.indices[i]] = 1;
        }

        int64_t write_ptr = 0;

        // 2. Hardware-Aware Culling
        for (int64_t i = 0; i < original_count; ++i) {
            const int64_t current_idx = r_selection.data.indices[i];
            bool is_border = false;

            if constexpr (T_Strategy::dimensions == 1) {
                int64_t neighbors[2] = {current_idx - 1, current_idx + 1};
                
                for (int64_t nx : neighbors) {
                    if (nx < 0 || nx >= total_capacity || is_selected[nx] == 0) {
                        is_border = true;
                        break;
                    }
                }
            } 
            else if constexpr (T_Strategy::dimensions == 2) {
                const int64_t width = p_view.strategy.stride_y / stride;
                const int64_t height = total_capacity / width;
                
                // Deconstruct ONCE per element, eliminating modulo from the dimension loop
                const int64_t x = current_idx % width;
                const int64_t y = current_idx / width;

                int64_t neighbors[4][2] = {
                    {x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}
                };

                for (auto& n : neighbors) {
                    if (n[0] < 0 || n[0] >= width || n[1] < 0 || n[1] >= height) {
                        is_border = true; break;
                    }
                    if (is_selected[p_view.strategy.get_index_2d(n[0], n[1], stride)] == 0) {
                        is_border = true; break;
                    }
                }
            }
            else if constexpr (T_Strategy::dimensions == 3) {
                const int64_t width = p_view.strategy.stride_y / stride;
                const int64_t height = p_view.strategy.stride_z / p_view.strategy.stride_y;
                const int64_t depth = total_capacity / (width * height);
                
                const int64_t x = current_idx % width;
                const int64_t y = (current_idx / width) % height;
                const int64_t z = current_idx / (width * height);

                int64_t neighbors[6][3] = {
                    {x - 1, y, z}, {x + 1, y, z}, {x, y - 1, z}, {x, y + 1, z}, {x, y, z - 1}, {x, y, z + 1}
                };

                for (auto& n : neighbors) {
                    if (n[0] < 0 || n[0] >= width || n[1] < 0 || n[1] >= height || n[2] < 0 || n[2] >= depth) {
                        is_border = true; break;
                    }
                    if (is_selected[p_view.strategy.get_index_3d(n[0], n[1], n[2], stride)] == 0) {
                        is_border = true; break;
                    }
                }
            }
            else if constexpr (T_Strategy::dimensions == 4) {
                const int64_t width = p_view.strategy.stride_y / stride;
                const int64_t height = p_view.strategy.stride_z / p_view.strategy.stride_y;
                const int64_t depth = p_view.strategy.stride_w / p_view.strategy.stride_z;
                const int64_t w_size = total_capacity / (width * height * depth);
                
                const int64_t x = current_idx % width;
                const int64_t y = (current_idx / width) % height;
                const int64_t z = (current_idx / (width * height)) % depth;
                const int64_t w = current_idx / (width * height * depth);

                int64_t neighbors[8][4] = {
                    {x - 1, y, z, w}, {x + 1, y, z, w}, {x, y - 1, z, w}, {x, y + 1, z, w},
                    {x, y, z - 1, w}, {x, y, z + 1, w}, {x, y, z, w - 1}, {x, y, z, w + 1}
                };

                for (auto& n : neighbors) {
                    if (n[0] < 0 || n[0] >= width || n[1] < 0 || n[1] >= height || n[2] < 0 || n[2] >= depth || n[3] < 0 || n[3] >= w_size) {
                        is_border = true; break;
                    }
                    if (is_selected[p_view.strategy.get_index_4d(n[0], n[1], n[2], n[3], stride)] == 0) {
                        is_border = true; break;
                    }
                }
            }

            // Halo Stripping: Retain only strict interior elements.
            if (!is_border) {
                r_selection.data.indices[write_ptr++] = current_idx;
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_dilation(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        const int64_t total_capacity = r_selection.capacity;
        const auto& part = p_view.grant->parts[p_view.grant_part_index];
        const size_t stride = part.element_stride;

        // 1. O(1) Transient Lookup Map Allocation
        uint8_t* is_selected = static_cast<uint8_t*>(p_context.local_workspace);
        for (int64_t i = 0; i < total_capacity; ++i) is_selected[i] = 0;
        
        const int64_t original_count = r_selection.element_count;
        const uint64_t* unclaimed = r_selection.unclaimed_mask;

        for (int64_t i = 0; i < original_count; ++i) {
            is_selected[r_selection.data.indices[i]] = 1;
        }

        // 2. Hardware-Aware Dilation
        for (int64_t i = 0; i < original_count; ++i) {
            const int64_t current_idx = r_selection.data.indices[i];

            if constexpr (T_Strategy::dimensions == 1) {
                int64_t neighbors[2] = {current_idx - 1, current_idx + 1};
                
                for (int64_t nx : neighbors) {
                    if (nx >= 0 && nx < total_capacity && is_selected[nx] == 0) {
                        if (unclaimed && (unclaimed[nx >> 6] & (1ULL << (nx & 63)))) {
                            p_context.queue_selection_command(target_buffer_id, nx);
                            is_selected[nx] = 1; // Prevent duplicate queues from adjacent cells
                        }
                    }
                }
            } 
            else if constexpr (T_Strategy::dimensions == 2) {
                const int64_t width = p_view.strategy.stride_y / stride;
                const int64_t height = total_capacity / width;
                
                const int64_t x = current_idx % width;
                const int64_t y = current_idx / width;

                int64_t neighbors[4][2] = {
                    {x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}
                };

                for (auto& n : neighbors) {
                    if (n[0] < 0 || n[0] >= width || n[1] < 0 || n[1] >= height) continue;
                    
                    int64_t neigh_idx = p_view.strategy.get_index_2d(n[0], n[1], stride);
                    if (is_selected[neigh_idx] == 0) {
                        if (unclaimed && (unclaimed[neigh_idx >> 6] & (1ULL << (neigh_idx & 63)))) {
                            p_context.queue_selection_command(target_buffer_id, neigh_idx);
                            is_selected[neigh_idx] = 1; 
                        }
                    }
                }
            }
            else if constexpr (T_Strategy::dimensions == 3) {
                const int64_t width = p_view.strategy.stride_y / stride;
                const int64_t height = p_view.strategy.stride_z / p_view.strategy.stride_y;
                const int64_t depth = total_capacity / (width * height);
                
                const int64_t x = current_idx % width;
                const int64_t y = (current_idx / width) % height;
                const int64_t z = current_idx / (width * height);

                int64_t neighbors[6][3] = {
                    {x - 1, y, z}, {x + 1, y, z}, {x, y - 1, z}, {x, y + 1, z}, {x, y, z - 1}, {x, y, z + 1}
                };

                for (auto& n : neighbors) {
                    if (n[0] < 0 || n[0] >= width || n[1] < 0 || n[1] >= height || n[2] < 0 || n[2] >= depth) continue;
                    
                    int64_t neigh_idx = p_view.strategy.get_index_3d(n[0], n[1], n[2], stride);
                    if (is_selected[neigh_idx] == 0) {
                        if (unclaimed && (unclaimed[neigh_idx >> 6] & (1ULL << (neigh_idx & 63)))) {
                            p_context.queue_selection_command(target_buffer_id, neigh_idx);
                            is_selected[neigh_idx] = 1;
                        }
                    }
                }
            }
            else if constexpr (T_Strategy::dimensions == 4) {
                const int64_t width = p_view.strategy.stride_y / stride;
                const int64_t height = p_view.strategy.stride_z / p_view.strategy.stride_y;
                const int64_t depth = p_view.strategy.stride_w / p_view.strategy.stride_z;
                const int64_t w_size = total_capacity / (width * height * depth);
                
                const int64_t x = current_idx % width;
                const int64_t y = (current_idx / width) % height;
                const int64_t z = (current_idx / (width * height)) % depth;
                const int64_t w = current_idx / (width * height * depth);

                int64_t neighbors[8][4] = {
                    {x - 1, y, z, w}, {x + 1, y, z, w}, {x, y - 1, z, w}, {x, y + 1, z, w},
                    {x, y, z - 1, w}, {x, y, z + 1, w}, {x, y, z, w - 1}, {x, y, z, w + 1}
                };

                for (auto& n : neighbors) {
                    if (n[0] < 0 || n[0] >= width || n[1] < 0 || n[1] >= height || n[2] < 0 || n[2] >= depth || n[3] < 0 || n[3] >= w_size) continue;
                    
                    int64_t neigh_idx = p_view.strategy.get_index_4d(n[0], n[1], n[2], n[3], stride);
                    if (is_selected[neigh_idx] == 0) {
                        if (unclaimed && (unclaimed[neigh_idx >> 6] & (1ULL << (neigh_idx & 63)))) {
                            p_context.queue_selection_command(target_buffer_id, neigh_idx);
                            is_selected[neigh_idx] = 1;
                        }
                    }
                }
            }
        }
    }
};

} // namespace ideam::core