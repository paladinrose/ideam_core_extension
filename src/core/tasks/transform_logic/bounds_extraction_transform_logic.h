#pragma once

#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <limits>
#include <type_traits>

namespace ideam::core {

template <typename T>
struct BoundsResult {
    T min_bounds;
    T max_bounds;
    T centroid;
    int64_t element_count = 0;
};

/**
 * BoundsExtractionTransformLogic<T>
 * Pure mathematical transform to find the AABB and Centroid of a selection.
 */
template <typename T>
struct alignas(64) BoundsExtractionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR; // Upgraded from restricted subset
    static constexpr DataType required_types              = DataType::ANY_NUMERIC | DataType::GODOT_VECTOR_TYPES;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;

    // --- Configuration ---
    uint32_t target_buffer_id = INVALID_ID;
    BoundsResult<T>* output_destination = nullptr;

    static godot::Array get_ui_properties() {
        return godot::Array(); 
    }
    
    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        
    }
    
    // --- The Transform Execution ---
    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD& context, const T_View& main_view) const {
        if (!output_destination) return;

        const MemoryBufferSelectionPOD* sel = context.get_selection(target_buffer_id);
        if (!sel || !sel->is_valid()) return;

        BoundsResult<T> local_result;
        _initialize_bounds(local_result);

        if (sel->mode == SelectionMode::DENSE) {
            _reduce_dense(*sel, main_view, local_result);
        } else if (sel->mode == SelectionMode::SPARSE) {
            _reduce_sparse(*sel, main_view, local_result);
        } else if (sel->mode == SelectionMode::RANGE) {
            _reduce_range(*sel, main_view, local_result);
        }

        if (local_result.element_count > 0) {
            local_result.centroid = (local_result.min_bounds + local_result.max_bounds) / static_cast<T>(2.0);
        }

        // Lock-free write to the pre-assigned output port
        *output_destination = local_result;
    }

private:
    inline void _initialize_bounds(BoundsResult<T>& r_res) const {
        if constexpr (std::is_arithmetic_v<T>) {
            r_res.min_bounds = std::numeric_limits<T>::max();
            r_res.max_bounds = std::numeric_limits<T>::lowest();
        } else {
            // Assumes vector types have a filled constructor or constants
            r_res.min_bounds = T(std::numeric_limits<float>::max());
            r_res.max_bounds = T(std::numeric_limits<float>::lowest());
        }
        r_res.centroid = T{};
        r_res.element_count = 0;
    }

    inline void _accumulate(BoundsResult<T>& r_res, const T& p_val) const {
        // C++26 assume hints can be placed in these hot loops
        if constexpr (std::is_arithmetic_v<T>) {
            if (p_val < r_res.min_bounds) r_res.min_bounds = p_val;
            if (p_val > r_res.max_bounds) r_res.max_bounds = p_val;
        } else {
            // For vectors, requires component-wise min/max overloads
            r_res.min_bounds = min(r_res.min_bounds, p_val);
            r_res.max_bounds = max(r_res.max_bounds, p_val);
        }
    }

    template <typename T_View>
    inline void _reduce_dense(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, BoundsResult<T>& r_res) const {
        const uint64_t* bitset = r_sel.data.bitset;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                _accumulate(r_res, p_view[i]);
                r_res.element_count++;
            }
        }
    }

    template <typename T_View>
    inline void _reduce_sparse(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, BoundsResult<T>& r_res) const {
        const int64_t* indices = r_sel.data.indices;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            _accumulate(r_res, p_view[indices[i]]);
        }
        r_res.element_count = count;
    }

    template <typename T_View>
    inline void _reduce_range(const MemoryBufferSelectionPOD& r_sel, const T_View& p_view, BoundsResult<T>& r_res) const {
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            _accumulate(r_res, p_view[i]);
        }
        r_res.element_count = r_sel.element_count;
    }
};

} // namespace ideam::core

 // IDEAM_CORE_BOUNDS_EXTRACTION_TRANSFORM_LOGIC_H