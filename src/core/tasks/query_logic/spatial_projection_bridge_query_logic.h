#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstring>
#include <bit>

namespace ideam::core {

/**
 * SpatialProjectionBridgeQueryLogic<T_Coord, T_Strategy>
 * Target: Field/Grid. Source: Physical Objects (Entities, Mass).
 * Projects Source coordinates into a grid bitmask, then filters the Target Grid.
 * REQUIREMENT: T_View MUST be bound to the Source Buffer.
 * TRANSIENT MEMORY: Requires `((capacity + 63) / 64) * 8` bytes of workspace.
 */
template <typename T_Coord, typename T_Strategy>
struct SpatialProjectionBridgeQueryLogic {
    using ValueType       = T_Coord; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T_Coord, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS | ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY;
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::VECTOR4D;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0; // Dynamically allocated by Job Graph

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    const MemoryBufferSelectionPOD* source_selection = nullptr; // Physical objects
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary col_prop;
        col_prop["name"] = "column_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);
        
        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("column_id")) {
            column_id = static_cast<uint32_t>(p_props["column_id"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy_Inner>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!source_selection || !p_context.local_workspace) return;

        // 1. Build the O(1) projection bitmask directly in Transient Memory
        const int64_t words = (r_selection.capacity + 63) >> 6;
        const size_t bytes_needed = words * sizeof(uint64_t);
        uint64_t* projection_mask = static_cast<uint64_t*>(p_context.local_workspace);
        std::memset(projection_mask, 0, bytes_needed);
        
        if (source_selection->mode == SelectionMode::DENSE) {
            const uint64_t* src_bitset = source_selection->data.bitset;
            for (int64_t i = 0; i < source_selection->capacity; ++i) {
                if (src_bitset[i >> 6] & (1ULL << (i & 63))) {
                    // CORRECTED: Safe extraction
                    int64_t cell_id = p_view.strategy.get_cell_index(_read_view(p_view, i));
                    if (cell_id >= 0 && cell_id < r_selection.capacity) {
                        projection_mask[cell_id >> 6] |= (1ULL << (cell_id & 63));
                    }
                }
            }
        } else if (source_selection->mode == SelectionMode::SPARSE) {
            for (int64_t i = 0; i < source_selection->element_count; ++i) {
                int64_t src_idx = source_selection->data.indices[i];
                // CORRECTED: Safe extraction
                int64_t cell_id = p_view.strategy.get_cell_index(_read_view(p_view, src_idx));
                if (cell_id >= 0 && cell_id < r_selection.capacity) {
                    projection_mask[cell_id >> 6] |= (1ULL << (cell_id & 63));
                }
            }
        }

        // 2. Apply projection to Target Selection
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) {
                uint64_t* target_bits = r_selection.data.bitset;
                for (int64_t w = 0; w < words; ++w) {
                    uint64_t old_word = target_bits[w];
                    target_bits[w] &= projection_mask[w];
                    r_selection.element_count -= std::popcount(old_word ^ target_bits[w]);
                }
            }
        } else if constexpr (Op == QueryOp::ADD) {
            const uint64_t* unclaimed = r_selection.unclaimed_mask;
            for (int64_t w = 0; w < words; ++w) {
                uint64_t mask = projection_mask[w];
                if (unclaimed) mask &= unclaimed[w];
                
                while (mask != 0) {
                    int bit_index = std::countr_zero(mask);
                    int64_t global_index = (w << 6) + bit_index;
                    p_context.queue_selection_command(target_buffer_id, global_index);
                    mask &= (mask - 1); 
                }
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

 // IDEAM_CORE_SPATIAL_PROJECTION_BRIDGE_QUERY_LOGIC_H