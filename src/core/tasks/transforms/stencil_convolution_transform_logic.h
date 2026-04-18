#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <array>

namespace ideam::core {

/**
 * StencilConvolutionTransformLogic<T, T_Strategy, KernelSize>
 * Applies a custom convolution matrix (e.g., Blur, Edge Detect, Game of Life) across a spatial grid.
 * Requires SwapView to ensure State(T+1) writes do not corrupt State(T) reads.
 */
template <typename T, typename T_Strategy, size_t KernelSize>
struct alignas(64) StencilConvolutionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr TransformRequirement requirements = TransformRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType supported_types = DataType::ANY_NUMERIC;
    static constexpr size_t transient_workspace_bytes = 0;

    struct KernelNode {
        uint32_t axis;     // 0=X, 1=Y, 2=Z
        int32_t step;      // -1, 0, 1
        float weight;
    };

    // --- Configuration ---
    uint32_t grid_buffer_id = INVALID_ID;
    std::array<KernelNode, KernelSize> kernel;
    float center_weight = 1.0f;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const {
        return grid_buffer_id;
    }

    template <typename T_View, typename T_Strat_Inner>
    inline void execute_transform(const TaskContextPOD& context, T_View& main_view) const {
        const MemoryBufferSelectionPOD* sel = context.get_selection(grid_buffer_id);
        if (!sel || sel->mode != SelectionMode::DENSE) return;

        const int64_t count = sel->capacity;
        const uint64_t* bitset = sel->data.bitset;
        const T_Strategy& strategy = main_view.get_strategy();

        for (int64_t i = 0; i < count; ++i) {
            if (!(bitset[i >> 6] & (1ULL << (i & 63)))) continue;

            T accumulator = main_view.get_current(i) * center_weight;

            // Apply Stencil Convolution
            for (size_t k = 0; k < KernelSize; ++k) {
                int64_t neighbor_idx = strategy.get_neighbor_index(i, kernel[k].axis, kernel[k].step);
                if (neighbor_idx >= 0 && neighbor_idx < count) {
                    accumulator += main_view.get_current(neighbor_idx) * kernel[k].weight;
                }
            }

            // Write to State T+1 safely
            main_view[i] = accumulator;
        }
    }
};

} // namespace ideam::core
 // IDEAM_CORE_STENCIL_CONVOLUTION_TRANSFORM_LOGIC_H