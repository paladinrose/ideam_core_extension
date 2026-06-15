#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h" // Upgraded to Static Stencil
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cstring>
#include <bit>

namespace ideam::core {

/**
 * MorphologicalStaticQueryLogic
 * Replaces both BorderCull and ErosionCull.
 * CULL (Erosion): Removes elements that touch inactive space.
 * ADD (Dilation): Queues inactive elements that touch active space.
 * * DESIGN: Utilizes StaticStencilView for pure DOD byte-offset traversal. 
 * Expected to be sized using `stencil_math::von_neumann_size`. 
 * Note: Edges will spatially wrap. If a borderless topology is not desired, 
 * either pad the grid with a Halo, or chain BorderQueryLogic<CULL> after this pass.
 */
template <typename T, typename T_Strategy, size_t PointCount>
struct MorphologicalStaticQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T, T_Strategy, PointCount>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::STENCIL_ACCESS | ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size = PointCount;
    
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
        iter_prop["hint_string"] = "1,100,1"; 
        props.push_back(iter_prop);

        return props;
    }

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("iterations")) {
            iterations = static_cast<int32_t>(p_props["iterations"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strat>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (r_selection.mode != SelectionMode::DENSE) return;
        if (!p_context.local_workspace) return;

        if constexpr (Op == QueryOp::CULL) {
            _execute_erosion(r_selection, p_view, p_context.local_workspace, p_context);
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
        if (p_index < 0 || p_index >= p_capacity) return false; 
        return (p_bitset[p_index >> 6] & (1ULL << (p_index & 63))) != 0;
    }

    template <typename T_View>
    void _execute_erosion(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, void* p_workspace, const TaskContextPOD& p_ctx) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t capacity = r_selection.capacity;
        
        uint64_t* snapshot = static_cast<uint64_t*>(p_workspace);
        const size_t bytes_needed = ((capacity + 63) / 64) * sizeof(uint64_t);

        // Hardware pointers for the DSU index-resolution trick
        const GrantPartPOD* part = p_ctx.get_grant_part(target_buffer_id);
        const intptr_t stride = part->element_stride;
        const uint8_t* raw_base = static_cast<const uint8_t*>(part->raw_base_ptr);

        for (int32_t iter = 0; iter < iterations; ++iter) {
            std::memcpy(snapshot, bitset, bytes_needed);

            for (int64_t i = 0; i < capacity; ++i) {
                if (!(snapshot[i >> 6] & (1ULL << (i & 63)))) continue;

                // Focus the View's mutable cursor
                p_view[i];

                bool should_erode = false;

                if constexpr (PointCount > 0) {
                    for (size_t p = 0; p < PointCount; ++p) {
                        const T& neigh_val = p_view.neighbor(p);
                        
                        // O(1) mathematical conversion back to global flat index
                        const intptr_t byte_diff = reinterpret_cast<const uint8_t*>(&neigh_val) - raw_base;
                        const int64_t neighbor_idx = byte_diff / stride;

                        if (!_evaluate_neighbor(neighbor_idx, snapshot, capacity)) {
                            should_erode = true;
                            break;
                        }
                    }
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

        const GrantPartPOD* part = p_ctx.get_grant_part(target_buffer_id);
        const intptr_t stride = part->element_stride;
        const uint8_t* raw_base = static_cast<const uint8_t*>(part->raw_base_ptr);

        for (int64_t i = 0; i < capacity; ++i) {
            if (!(bitset[i >> 6] & (1ULL << (i & 63)))) continue;

            p_view[i]; 

            if constexpr (PointCount > 0) {
                for (size_t p = 0; p < PointCount; ++p) {
                    const T& neigh_val = p_view.neighbor(p);
                    
                    const intptr_t byte_diff = reinterpret_cast<const uint8_t*>(&neigh_val) - raw_base;
                    const int64_t neighbor_idx = byte_diff / stride;
                    
                    if (neighbor_idx >= 0 && neighbor_idx < capacity) {
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