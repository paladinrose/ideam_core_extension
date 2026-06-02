#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstring>
#include <bit>

namespace ideam::core {

/**
 * MorphologicalQueryLogic
 * Replaces both BorderCull and ErosionCull.
 * CULL (Erosion): Removes elements that touch inactive space.
 * ADD (Dilation): Queues inactive elements that touch active space.
 * Fully utilizes Transient Memory to perform safe, lock-free N-pass iterations.
 * 
 * Note: Requires the graph builder to set set_node_transient_requirement to
 * (capacity + 63) / 64 * 8 bytes for the snapshot buffer. This allows the logic to 
 * operate on a stable snapshot of the bitset during erosion passes, while dilation can 
 * safely operate directly on the live bitset due to its append-only nature.
 */
template <typename T, typename T_Strategy>
struct MorphologicalQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StencilView<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::STENCIL_ACCESS | ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY;
    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;
    int32_t iterations = 1;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary iter_prop;
        iter_prop["name"] = "iterations";
        iter_prop["type"] = godot::Variant::INT;
        iter_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        iter_prop["hint_string"] = "1,100,1"; // Safe bounds for erosion passes
        props.push_back(iter_prop);

        return props;
    }

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("iterations")) {
            iterations = p_props["iterations"];
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        // This logic requires a dense bitset to perform boundary math
        if (r_selection.mode != SelectionMode::DENSE) return;

        // Ensure transient workspace was provided by the Graph
        if (!p_context.local_workspace) return;

        if constexpr (Op == QueryOp::CULL) {
            _execute_erosion(r_selection, p_view, p_context.local_workspace);
        } else if constexpr (Op == QueryOp::ADD) {
            _execute_dilation(r_selection, p_view, p_context);
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate_neighbor(int64_t p_index, const uint64_t* p_bitset, int64_t p_capacity) const {
        if (p_index < 0 || p_index >= p_capacity) return false; // Out of bounds is "empty"
        return (p_bitset[p_index >> 6] & (1ULL << (p_index & 63))) != 0;
    }

    template <typename T_View>
    void _execute_erosion(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, void* p_workspace) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t capacity = r_selection.capacity;
        const T_Strategy& strategy = p_view.get_strategy();
        
        uint64_t* snapshot = static_cast<uint64_t*>(p_workspace);
        const size_t bytes_needed = ((capacity + 63) / 64) * sizeof(uint64_t);

        for (int32_t iter = 0; iter < iterations; ++iter) {
            // Snapshot current state into transient L1 cache
            std::memcpy(snapshot, bitset, bytes_needed);

            for (int64_t i = 0; i < capacity; ++i) {
                if (!(snapshot[i >> 6] & (1ULL << (i & 63)))) continue;

                bool should_erode = false;

                for (uint32_t d = 0; d < T_Strategy::dimensions; ++d) {
                    for (int32_t step : {-1, 1}) {
                        int64_t neighbor_idx = strategy.get_neighbor_index(i, d, step);
                        if (!_evaluate_neighbor(neighbor_idx, snapshot, capacity)) {
                            should_erode = true;
                            break;
                        }
                    }
                    if (should_erode) break;
                }

                if (should_erode) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
            if (r_selection.element_count == 0) break;
        }
    }

    template <typename T_View>
    void _execute_dilation(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* bitset = r_selection.data.bitset;
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        const int64_t capacity = r_selection.capacity;
        const T_Strategy& strategy = p_view.get_strategy();

        // Note: Wave command queueing is automatically deferred by the graph, 
        // so Dilation doesn't strictly need a snapshot for a single pass!
        for (int64_t i = 0; i < capacity; ++i) {
            if (!(bitset[i >> 6] & (1ULL << (i & 63)))) continue;

            for (uint32_t d = 0; d < T_Strategy::dimensions; ++d) {
                for (int32_t step : {-1, 1}) {
                    int64_t neighbor_idx = strategy.get_neighbor_index(i, d, step);
                    
                    if (neighbor_idx >= 0 && neighbor_idx < capacity) {
                        // If neighbor is inactive but globally unclaimed, queue it!
                        if (!(bitset[neighbor_idx >> 6] & (1ULL << (neighbor_idx & 63)))) {
                            if (unclaimed && (unclaimed[neighbor_idx >> 6] & (1ULL << (neighbor_idx & 63)))) {
                                p_ctx.queue_selection_command(target_buffer_id, neighbor_idx);
                            }
                        }
                    }
                }
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_MORPHOLOGICAL_QUERY_LOGIC_H