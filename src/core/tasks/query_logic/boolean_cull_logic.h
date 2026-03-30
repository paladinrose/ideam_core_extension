#ifndef IDEAM_CORE_BOOLEAN_CULL_LOGIC_H
#define IDEAM_CORE_BOOLEAN_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

namespace ideam::core {

/**
 * BooleanCullLogic<T>
 * A specialized logic for truth-value filtering.
 * T: The underlying storage type (bool, uint8_t, or int32_t for aligned bools).
 */
template <typename T>
struct BooleanCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // Pure linear evaluation; no special hardware requirements.
    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    // --- Configuration Data ---
    uint32_t column_id   = 0; 
    bool target_value    = true;

    /**
     * execute_cull
     * Evaluates the boolean condition and prunes the selection accordingly.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            _cull_sparse(r_selection, p_view);
        }
    }

private:
    /**
     * _evaluate
     * Core equality check. FORCE_INLINE ensures this disappears into the loop.
     */
    [[nodiscard]] FORCE_INLINE bool _evaluate(T p_val) const {
        return static_cast<bool>(p_val) == target_value;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        for (int64_t i = 0; i < count; ++i) {
            // Only evaluate elements already in the selection
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(p_view[i])) {
                    // Prune from bitset
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            if (_evaluate(p_view[idx])) {
                // Keep the index and advance the write pointer (In-place filter)
                indices[write_ptr++] = idx;
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_BOOLEAN_CULL_LOGIC_H