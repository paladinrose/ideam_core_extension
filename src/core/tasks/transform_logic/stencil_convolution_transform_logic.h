#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h" // Upgraded View
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <array>

namespace ideam::core {

template <typename T, typename T_Strategy, size_t KernelSize>
struct alignas(64) StencilConvolutionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    // We use StaticStencilView for high-speed reads
    using DefaultView     = StaticStencilView<T, DefaultStrategy, KernelSize>;

    static constexpr TransformRequirement requirements = TransformRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType supported_types = DataType::ANY_NUMERIC;
    static constexpr size_t transient_workspace_bytes = 0;

    // --- Configuration ---
    uint32_t grid_buffer_id = INVALID_ID;
    
    // Abstract spatial deltas [dx, dy, dz]. Aligns with Strategy dimensionality.
    std::array<std::array<int64_t, T_Strategy::dimensions>, KernelSize> kernel_deltas;
    std::array<float, KernelSize> kernel_weights;
    float center_weight = 1.0f;

    // --- Phase 0 Cache (Mutated on Main Thread) ---
    std::array<intptr_t, KernelSize> baked_offsets{};
    uint64_t last_manager_version = 0;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const {
        return grid_buffer_id;
    }

    /**
     * prepare (Phase 0)
     * Executes single-threaded. Bakes the byte-offsets for the active memory topology.
     */
    inline void prepare(const TaskContextPOD& context) {
        if (context.manager->version != last_manager_version) {
            const GrantPartPOD* part = context.get_grant_part(grid_buffer_id);
            if (!part) return;

            // Extract the strategy configuration from your manager/context
            // Assuming T_Strategy can be default-constructed or fetched
            T_Strategy strategy; 
            
            baked_offsets = DefaultView::bake_spatial_offsets(strategy, part->element_stride, kernel_deltas);
            last_manager_version = context.manager->version;
        }
    }

    /**
     * configure_view (Phase 1 Init)
     * Injects the pre-baked cache directly into the View's stack footprint.
     */
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& view, const TaskContextPOD& context, const GrantPartPOD* part) const {
        if constexpr (requires { view.baked_offsets; }) {
            view.baked_offsets = baked_offsets;
        }
    }

    /**
     * execute_transform (Phase 1 Hot Loop)
     */
    template <typename T_View, typename T_Strat_Inner>
    inline void execute_transform(const TaskContextPOD& context, T_View& main_view) const {
        const MemoryBufferSelectionPOD* sel = context.get_selection(grid_buffer_id);
        if (!sel || sel->mode != SelectionMode::DENSE) return;

        const int64_t count = sel->capacity;
        const uint64_t* bitset = sel->data.bitset;
        
        // Retrieve a raw write pointer for State T+1 (assuming Swap Buffer setup)
        // main_view handles reading from State T.
        const GrantPartPOD* write_part = context.get_grant_part(grid_buffer_id); // Adjust for your swap logic
        T* write_ptr = reinterpret_cast<T*>(write_part->raw_base_ptr);

        for (int64_t i = 0; i < count; ++i) {
            if (!(bitset[i >> 6] & (1ULL << (i & 63)))) continue;

            // 1. Focus the Stencil on the current index (Calculates center pointer once)
            main_view[i];

            // 2. Read Center
            T accumulator = main_view.center() * center_weight;

            // 3. Unrolled, ALU-Bypassing Neighbor Convolution
            for (size_t k = 0; k < KernelSize; ++k) {
                // main_view.neighbor(k) expands to: *(center_ptr + baked_offsets[k])
                // Zero multiplication. Zero dimension math. Just a pure memory fetch.
                accumulator += main_view.neighbor(k) * kernel_weights[k];
            }

            // 4. Commit to Next State
            write_ptr[i] = accumulator;
        }
    }
};

} // namespace ideam::core