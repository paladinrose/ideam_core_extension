#ifndef IDEAM_CORE_COMPONENT_CULL_LOGIC_H
#define IDEAM_CORE_COMPONENT_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/sparse_set_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

namespace ideam::core {

/**
 * ComponentCullLogic
 * A "Structural Query" logic that prunes entities missing a specific component.
 * Operates on SparseSet buffers where the element value is the Entity ID.
 */
struct ComponentCullLogic {
    // --- View Binding & Logic Traits ---
    // The ValueType for a SparseSet structural query is the Entity ID (uint32_t).
    using ValueType       = uint32_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SparseSetView<ValueType, DefaultStrategy>;

    // No spatial folding or SIMD required; just a topological presence check.
    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::SPARSE_SET;

    // --- Configuration Data ---
    uint32_t component_buffer_id = 0; // The ID of the component pool to check for

    /**
     * execute_cull
     * Evaluates entity presence and updates the selection.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view, p_context);
        } else {
            _cull_sparse(r_selection, p_view, p_context);
        }
    }

private:
    /**
     * _evaluate
     * Uses the MemoryManager utility to check if the entity exists in the target buffer.
     */
    inline bool _evaluate(uint32_t p_entity_id, const TaskContextPOD& p_context) const {
        // O(1) topological check via the manager's sparse-array lookup
        return p_context.manager->buffer_contains_id(component_buffer_id, p_entity_id);
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        for (int64_t i = 0; i < count; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                // p_view[i] returns the EntityID stored in the dense array
                if (!_evaluate(p_view[i], p_ctx)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t dense_idx = indices[i];
            // Resolve the EntityID via the view and check presence
            if (_evaluate(p_view[dense_idx], p_ctx)) {
                indices[write_ptr++] = dense_idx;
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_COMPONENT_CULL_LOGIC_H