#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

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
 * ValueAccumulationTransformLogic<T>
 * High-speed pure math reduction kernel for numeric properties.
 */
template <typename T>
struct alignas(64) ValueAccumulationTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR; // Upgraded from FLAT | AOS | SOA
    static constexpr DataType required_types              = DataType::ANY_NUMERIC;        // Cleaned up to use aggregate mask
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;

    // --- Configuration ---
    AccumulationMode mode = AccumulationMode::SUM;
    uint32_t primary_buffer_id = INVALID_ID;
    AccumulationResult* output_destination = nullptr;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        // Map the C++ enum sequence strictly to the Godot dropdown index
        mode_prop["hint_string"] = "Sum,Average,Min,Max,Count Non Zero";
        props.push_back(mode_prop);

        return props;
    }
    
    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return primary_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("mode")) {
            mode = static_cast<AccumulationMode>(static_cast<uint8_t>(p_props["mode"]));
        }
    }
        
    
    // --- The Transform Execution ---
    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD& context, const T_View& main_view) const {
        if (!output_destination) return;

        const MemoryBufferSelectionPOD* sel = context.get_selection(primary_buffer_id);
        if (!sel || !sel->is_valid()) return;

        double local_acc = 0.0;
        if (mode == AccumulationMode::MIN) local_acc = std::numeric_limits<double>::max();
        if (mode == AccumulationMode::MAX) local_acc = std::numeric_limits<double>::lowest();

        // Branchless dispatch to the correct mathematical loop
        switch (mode) {
            case AccumulationMode::SUM:            _dispatch<AccumulationMode::SUM>(*sel, main_view, local_acc); break;
            case AccumulationMode::AVERAGE:        _dispatch<AccumulationMode::AVERAGE>(*sel, main_view, local_acc); break;
            case AccumulationMode::MIN:            _dispatch<AccumulationMode::MIN>(*sel, main_view, local_acc); break;
            case AccumulationMode::MAX:            _dispatch<AccumulationMode::MAX>(*sel, main_view, local_acc); break;
            case AccumulationMode::COUNT_NON_ZERO: _dispatch<AccumulationMode::COUNT_NON_ZERO>(*sel, main_view, local_acc); break;
        }

        if (mode == AccumulationMode::AVERAGE && sel->element_count > 0) {
            local_acc /= static_cast<double>(sel->element_count);
        }

        output_destination->value = local_acc;
        output_destination->count = sel->element_count;
    }

private:
    template <AccumulationMode M, typename T_View>
    inline void _dispatch(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, double& r_acc) const {
        if (r_sel.mode == SelectionMode::DENSE)        _loop_dense<M>(r_sel, p_view, r_acc);
        else if (r_sel.mode == SelectionMode::SPARSE)  _loop_sparse<M>(r_sel, p_view, r_acc);
        else if (r_sel.mode == SelectionMode::RANGE)   _loop_range<M>(r_sel, p_view, r_acc);
    }

    template <AccumulationMode M>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void _step(double& r_acc, double p_val) const {
        if constexpr (M == AccumulationMode::SUM || M == AccumulationMode::AVERAGE) r_acc += p_val;
        else if constexpr (M == AccumulationMode::MIN) { if (p_val < r_acc) r_acc = p_val; }
        else if constexpr (M == AccumulationMode::MAX) { if (p_val > r_acc) r_acc = p_val; }
        else if constexpr (M == AccumulationMode::COUNT_NON_ZERO) { if (p_val != 0.0) r_acc += 1.0; }
    }

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

 // IDEAM_CORE_VALUE_ACCUMULATION_TRANSFORM_LOGIC_H