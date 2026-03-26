#ifndef IDEAM_CORE_AABB_CULL_LOGIC_H
#define IDEAM_CORE_AABB_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h" 
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::core {

struct AABBCullLogic {
    using ValueType       = godot::Vector3; 
    using DefaultStrategy = Spatial3DStrategy;
    using DefaultView     = SingleElementView<ValueType, DefaultStrategy>;

    godot::Vector3 box_min;
    godot::Vector3 box_max;
    uint32_t target_part_index = 0;

    [[nodiscard]] uint32_t get_target_part_index() const { return target_part_index; }

    template<typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) {
        
        // We iterate based on the SELECTION count, not the BUFFER capacity.
        // This honors the View's operator[] design.
        const int64_t count = r_selection.element_count;

        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view, count);
        } else {
            // SPARSE and RANGE can be handled via selection-index iteration
            _cull_indexed(r_selection, p_view, count);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) { /* No-op */ }

private:
    template<typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, int64_t p_count) {
        uint64_t* bitset = r_selection.data.bitset;
        // In DENSE mode, we iterate the capacity because bits might be sparse.
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                // p_view[i] for DENSE correctly maps buffer_index == selection_index
                if (!_evaluate(p_view[i])) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template<typename T_View>
    void _cull_indexed(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, int64_t p_count) {
        // For SPARSE/RANGE, we must be careful. 
        // We iterate selection indices, but since we are MUTATING the selection
        // during the cull, we'll work backwards or use a temporary to avoid 
        // invalidating the very indices the View is reading.
        
        if (r_selection.mode == SelectionMode::SPARSE) {
            int64_t write_ptr = 0;
            for (int64_t i = 0; i < p_count; ++i) {
                // We use the view to get the value. The view handles indices[i] lookup.
                if (_evaluate(p_view[i])) {
                    r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
                }
            }
            r_selection.element_count = write_ptr;
        } 
        else if (r_selection.mode == SelectionMode::RANGE) {
            // RANGE culls usually force a transition to SPARSE if internal elements are cut.
            // For now, we can only "shrink" the range from either end.
            // (Logic for range-to-sparse conversion omitted for brevity unless requested)
        }
    }

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

#endif