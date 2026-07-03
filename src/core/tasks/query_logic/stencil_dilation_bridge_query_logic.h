#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstring>
#include <bit>

namespace ideam::core {

/**
 * StencilDilationBridgeQueryLogic<T_Coord, T_Strategy>
 * Target: Grid (ANY_SPATIAL). Source: Entities (SPARSE_SET).
 * Maps Entity coordinates to a Grid, then applies a dynamic Stencil to activate surrounding cells.
 * REQUIREMENT: T_View MUST be bound to the Source Buffer.
 * TRANSIENT MEMORY: Requires `((capacity + 63) / 64) * 8` bytes of workspace.
 */
template <typename T_Coord, typename T_Strategy>
struct StencilDilationBridgeQueryLogic {
    using ValueType       = T_Coord; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StencilView<T_Coord, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::STENCIL_ACCESS | ViewCapability::SPATIAL_ACCESS | ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::VECTOR4I | DataType::VECTOR4D;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = T_Strategy::dimensions; 
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0; // Dynamically allocated by Job Graph

    static constexpr bool supports_cull = false; // Stencils are used for dilation/addition
    static constexpr bool supports_addition = true;

    static constexpr std::string_view display_name = "Stencil Dilation Bridge";
    
    int32_t radius = 1;

    const MemoryBufferSelectionPOD* source_selection = nullptr; 
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary rad_prop;
        rad_prop["name"] = "radius";
        rad_prop["type"] = godot::Variant::INT;
        rad_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rad_prop);

        godot::Dictionary col_prop;
        col_prop["name"] = "column_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);
        
        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("radius")) {
            radius = static_cast<int32_t>(static_cast<int64_t>(p_props["radius"]));
        }
        if (p_props.has("column_id")) {
            column_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["column_id"]));
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy_Inner>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!source_selection || Op != QueryOp::ADD || !p_context.local_workspace) return;

        const int64_t target_capacity = r_selection.capacity;
        const int64_t words = (target_capacity + 63) >> 6;
        const size_t bytes_needed = words * sizeof(uint64_t);
        
        // 1. Build the dilated projection mask locally in Transient Memory
        uint64_t* dilation_mask = static_cast<uint64_t*>(p_context.local_workspace);
        std::memset(dilation_mask, 0, bytes_needed);

        const T_Strategy& strategy = p_view.strategy;
        const GrantPartPOD* target_part = p_context.get_grant_part(target_buffer_id);
        const intptr_t target_stride = target_part->element_stride;

        constexpr size_t dim = T_Strategy::dimensions;

        auto apply_stencil_to_mask = [&](int64_t src_idx) {
            T_Coord center_coord = _read_view(p_view, src_idx);
            int64_t center_cell = strategy.get_cell_index(center_coord);
            
            if (center_cell < 0 || center_cell >= target_capacity) return;

            if constexpr (dim == 1) {
                for (int32_t dx = -radius; dx <= radius; ++dx) {
                    int64_t nx = center_cell + dx;
                    if (nx >= 0 && nx < target_capacity) {
                        dilation_mask[nx >> 6] |= (1ULL << (nx & 63));
                    }
                }
            } 
            else if constexpr (dim == 2) {
                const int64_t width = strategy.stride_y / target_stride;
                const int64_t height = target_capacity / width;
                const int64_t cx = center_cell % width;
                const int64_t cy = center_cell / width;
                
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        int64_t nx = cx + dx;
                        int64_t ny = cy + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int64_t neigh_cell = nx + (ny * width);
                            dilation_mask[neigh_cell >> 6] |= (1ULL << (neigh_cell & 63));
                        }
                    }
                }
            }
            else if constexpr (dim == 3) {
                const int64_t width = strategy.stride_y / target_stride;
                const int64_t height = strategy.stride_z / strategy.stride_y;
                const int64_t depth = target_capacity / (width * height);
                
                const int64_t cx = center_cell % width;
                const int64_t cy = (center_cell / width) % height;
                const int64_t cz = center_cell / (width * height);

                for (int32_t dz = -radius; dz <= radius; ++dz) {
                    for (int32_t dy = -radius; dy <= radius; ++dy) {
                        for (int32_t dx = -radius; dx <= radius; ++dx) {
                            int64_t nx = cx + dx;
                            int64_t ny = cy + dy;
                            int64_t nz = cz + dz;
                            if (nx >= 0 && nx < width && ny >= 0 && ny < height && nz >= 0 && nz < depth) {
                                int64_t neigh_cell = nx + (ny * width) + (nz * width * height);
                                dilation_mask[neigh_cell >> 6] |= (1ULL << (neigh_cell & 63));
                            }
                        }
                    }
                }
            }
            else if constexpr (dim == 4) {
                const int64_t width = strategy.stride_y / target_stride;
                const int64_t height = strategy.stride_z / strategy.stride_y;
                const int64_t depth = strategy.stride_w / strategy.stride_z;
                const int64_t w_size = target_capacity / (width * height * depth);
                
                const int64_t cx = center_cell % width;
                const int64_t cy = (center_cell / width) % height;
                const int64_t cz = (center_cell / (width * height)) % depth;
                const int64_t cw = center_cell / (width * height * depth);

                for (int32_t dw = -radius; dw <= radius; ++dw) {
                    for (int32_t dz = -radius; dz <= radius; ++dz) {
                        for (int32_t dy = -radius; dy <= radius; ++dy) {
                            for (int32_t dx = -radius; dx <= radius; ++dx) {
                                int64_t nx = cx + dx;
                                int64_t ny = cy + dy;
                                int64_t nz = cz + dz;
                                int64_t nw = cw + dw;
                                if (nx >= 0 && nx < width && ny >= 0 && ny < height && nz >= 0 && nz < depth && nw >= 0 && nw < w_size) {
                                    int64_t neigh_cell = nx + (ny * width) + (nz * width * height) + (nw * width * height * depth);
                                    dilation_mask[neigh_cell >> 6] |= (1ULL << (neigh_cell & 63));
                                }
                            }
                        }
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