#ifndef IDEAM_CORE_ARCHETYPE_CULL_LOGIC_H
#define IDEAM_CORE_ARCHETYPE_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/sparse_set_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h" 
#include "query_logic_traits.h"
#include <vector>

namespace ideam::core {

/**
 * ArchetypeCullLogic
 * Filters a selection of entities based on the presence of required components.
 * This is a "Structural Query" that prunes entities not matching a component signature.
 */
struct ArchetypeCullLogic {
    // --- View Binding & Logic Traits ---
    // The "ValueType" of a SparseSet is usually the EntityID itself.
    using ValueType       = uint32_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SparseSetView<ValueType, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::SPARSE_SET;

    // --- Configuration Data ---
    static constexpr size_t MAX_SIGNATURE_SIZE = 16;
    
    uint32_t target_buffer_id = 0; // The primary Entity ID buffer
    uint32_t required_buffer_ids[MAX_SIGNATURE_SIZE] = {0};
    uint32_t required_count = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    /**
     * execute_cull
     * Iterates the current selection and validates the presence of the entity
     * across all required sparse component buffers.
     */
    template<typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) {
        
        const int64_t count = r_selection.element_count;

        // Optimized dispatch: Archetype checks on SparseSets benefit from 
        // identity-mapped DENSE iteration or index-based SPARSE iteration.
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view, p_context);
        } else {
            _cull_sparse(r_selection, p_view, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) { /* No-op */ }

private:
    /**
     * _evaluate
     * Core Archetype check. Uses the Manager's fast path to verify if an ID
     * exists in the sparse page table of all required components.
     */
    inline bool _evaluate(uint32_t p_entity_id, const TaskContextPOD& p_context) const {
        for (uint32_t i = 0; i < required_count; ++i) {
            // Context-driven lookup for component presence
            if (!p_context.manager->buffer_contains_id(required_buffer_ids[i], p_entity_id)) {
                return false;
            }
        }
        return true;
    }

    template<typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                // p_view[i] returns the EntityID for this dense slot
                if (!_evaluate(p_view[i], p_ctx)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template<typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) {
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;
        
        for (int64_t i = 0; i < count; ++i) {
            // p_view[i] maps the selection index to the EntityID
            if (_evaluate(p_view[i], p_ctx)) {
                r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_ARCHETYPE_CULL_LOGIC_H