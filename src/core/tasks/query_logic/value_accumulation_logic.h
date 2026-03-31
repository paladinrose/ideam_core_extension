#ifndef IDEAM_CORE_VALUE_ACCUMULATION_LOGIC_H
#define IDEAM_CORE_VALUE_ACCUMULATION_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <limits>
#include <type_traits>

namespace ideam::core {

enum class AccumulationMode : uint8_t {
    SUM,
    AVERAGE,
    MIN,
    MAX,
    COUNT_NON_ZERO
};

struct AccumulationResult {
    double value = 0.0;
    int64_t count = 0;
};

/**
 * ValueAccumulationLogic<T>
 * High-speed read-only reduction kernel for numeric properties.
 * Writes the final aggregated result to an output pointer (Graph Port).
 */
template <typename T>
struct ValueAccumulationLogic {
    static_assert(std::is_arithmetic_v<T>, "ValueAccumulationLogic strictly requires numeric scalar types.");

    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::READ_ONLY_DATA;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    // --- Configuration ---
    AccumulationMode mode = AccumulationMode::SUM;
    
    // Output Graph Port
    AccumulationResult* output_destination = nullptr;

    template <typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!output_destination) return;

        const GrantPartPOD* part = p_context.get_grant_part(p_view.grant_part_index);
        const MemoryBufferSelectionPOD& r_sel = part->selection;

        if (r_sel.element_count == 0) {
            *output_destination = {0.0, 0};
            return;
        }

        AccumulationResult local_result;
        local_result.count = r_sel.element_count;

        // Initialize bounds for MIN / MAX
        if (mode == AccumulationMode::MIN) local_result.value = std::numeric_limits<double>::max();
        else if (mode == AccumulationMode::MAX) local_result.value = std::numeric_limits<double>::lowest();
        else local_result.value = 0.0;

        // Dispatch based on topology
        if (r_sel.mode == SelectionMode::DENSE) {
            _dispatch_dense(r_sel, p_view, local_result.value);
        } else if (r_sel.mode == SelectionMode::SPARSE) {
            _dispatch_sparse(r_sel, p_view, local_result.value);
        } else if (r_sel.mode == SelectionMode::RANGE) {
            _dispatch_range(r_sel, p_view, local_result.value);
        }

        // Finalize Average calculation if needed
        if (mode == AccumulationMode::AVERAGE && local_result.count > 0) {
            local_result.value /= static_cast<double>(local_result.count);
        }

        // Write to Graph Port
        *output_destination = local_result;
    }

private:
    // --- Operation Dispatchers ---
    
    template <typename T_View>
    inline void _dispatch_dense(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        switch (mode) {
            case AccumulationMode::SUM:            
            case AccumulationMode::AVERAGE:        _loop_dense<AccumulationMode::SUM>(r_sel, p_view, r_acc); break;
            case AccumulationMode::MIN:            _loop_dense<AccumulationMode::MIN>(r_sel, p_view, r_acc); break;
            case AccumulationMode::MAX:            _loop_dense<AccumulationMode::MAX>(r_sel, p_view, r_acc); break;
            case AccumulationMode::COUNT_NON_ZERO: _loop_dense<AccumulationMode::COUNT_NON_ZERO>(r_sel, p_view, r_acc); break;
        }
    }

    template <typename T_View>
    inline void _dispatch_sparse(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        switch (mode) {
            case AccumulationMode::SUM:            
            case AccumulationMode::AVERAGE:        _loop_sparse<AccumulationMode::SUM>(r_sel, p_view, r_acc); break;
            case AccumulationMode::MIN:            _loop_sparse<AccumulationMode::MIN>(r_sel, p_view, r_acc); break;
            case AccumulationMode::MAX:            _loop_sparse<AccumulationMode::MAX>(r_sel, p_view, r_acc); break;
            case AccumulationMode::COUNT_NON_ZERO: _loop_sparse<AccumulationMode::COUNT_NON_ZERO>(r_sel, p_view, r_acc); break;
        }
    }

    template <typename T_View>
    inline void _dispatch_range(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        switch (mode) {
            case AccumulationMode::SUM:            
            case AccumulationMode::AVERAGE:        _loop_range<AccumulationMode::SUM>(r_sel, p_view, r_acc); break;
            case AccumulationMode::MIN:            _loop_range<AccumulationMode::MIN>(r_sel, p_view, r_acc); break;
            case AccumulationMode::MAX:            _loop_range<AccumulationMode::MAX>(r_sel, p_view, r_acc); break;
            case AccumulationMode::COUNT_NON_ZERO: _loop_range<AccumulationMode::COUNT_NON_ZERO>(r_sel, p_view, r_acc); break;
        }
    }

    // --- Math Accumulator Helper ---
    
    template <AccumulationMode M>
    static _FORCE_INLINE_ void _step(double& r_acc, double p_val) {
        if constexpr (M == AccumulationMode::SUM) r_acc += p_val;
        else if constexpr (M == AccumulationMode::MIN) r_acc = (p_val < r_acc) ? p_val : r_acc;
        else if constexpr (M == AccumulationMode::MAX) r_acc = (p_val > r_acc) ? p_val : r_acc;
        else if constexpr (M == AccumulationMode::COUNT_NON_ZERO) { if (p_val != 0.0) r_acc += 1.0; }
    }

    // --- Core High-Speed Loops ---

    template <AccumulationMode M, typename T_View>
    inline void _loop_dense(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        const uint64_t* bitset = r_sel.data.bitset;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                _step<M>(r_acc, static_cast<double>(p_view[i]));
            }
        }
    }

    template <AccumulationMode M, typename T_View>
    inline void _loop_sparse(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        const int64_t* indices = r_sel.data.indices;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            _step<M>(r_acc, static_cast<double>(p_view[indices[i]]));
        }
    }

    template <AccumulationMode M, typename T_View>
    inline void _loop_range(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            _step<M>(r_acc, static_cast<double>(p_view[i]));
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_VALUE_ACCUMULATION_LOGIC_H