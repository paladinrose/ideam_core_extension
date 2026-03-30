#ifndef IDEAM_CORE_AABB_CULL_LOGIC_H
#define IDEAM_CORE_AABB_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h" 
#include "query_logic_traits.h" // Established in previous architectural turn
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::core {

/**
 * AABBCullLogic
 * Performs a spatial intersection test between a 3D Box and the target buffer.
 * Mutates the Selection bitmask/indices to prune non-intersecting elements.
 */
struct AABBCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = godot::Vector3; 
    using DefaultStrategy = Spatial3DStrategy;
    using DefaultView     = SingleElementView<ValueType, DefaultStrategy>;

    // Meta for UI filtering and Task Graph validation
    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;

    // --- Configuration Data ---
    godot::Vector3 box_min;
    godot::Vector3 box_max;
    uint32_t target_buffer_id = 0; // Migrated from part_index for DOD stability

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    /**
     * execute_cull
     * The primary entry point for Querying. Honors the View's operator[] design
     * while mutating the selection bitmask or index array.
     */
    template<typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) {
        
        const int64_t count = r_selection.element_count;

        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view, count);
        } else {
            // SPARSE and RANGE can be handled via selection-index iteration
            _cull_indexed(r_selection, p_view, count);
        }
    }

    /**
     * execute_sim
     * No-op: Culling logic is strictly isolated from data mutation.
     */
    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) { /* No-op */ }

private:
    /**
     * _cull_dense
     * Iterates the bitset capacity to prune active bits.
     */
    template<typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, int64_t p_count) {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            // Check if bit is currently set before evaluating
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(p_view[i])) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    /**
     * _cull_indexed
     * Compacts the index array for SPARSE selections.
     */
    template<typename T_View>
    void _cull_indexed(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, int64_t p_count) {
        if (r_selection.mode == SelectionMode::SPARSE) {
            int64_t write_ptr = 0;
            for (int64_t i = 0; i < p_count; ++i) {
                // View[i] correctly resolves the actual buffer index via r_selection.data.indices[i]
                if (_evaluate(p_view[i])) {
                    r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
                }
            }
            r_selection.element_count = write_ptr;
        } 
        else if (r_selection.mode == SelectionMode::RANGE) {
            // Note: Range culls typically require a transition to SPARSE if internal 
            // elements are removed. For now, this logic assumes full removal or retention.
        }
    }

    /**
     * _evaluate
     * Core AABB intersection test. Inlined for zero-cost abstraction.
     */
    template<typename T>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_val) const {
        if constexpr (std::is_same_v<T, godot::Vector2> || std::is_same_v<T, godot::Vector2i>) {
            return (p_val.x >= box_min.x && p_val.x <= box_max.x) &&
                   (p_val.y >= box_min.y && p_val.y <= box_max.y);
        } else {
            return (p_val.x >= box_min.x && p_val.x <= box_max.x) &&
                   (p_val.y >= box_min.y && p_val.y <= box_max.y) &&
                   (p_val.z >= box_min.z && p_val.z <= box_max.z);
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_AABB_CULL_LOGIC_H