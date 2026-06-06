#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/swap_view.h" // Added for Write-Routing
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <array>
#include <new> // Required for placement new in the workspace

namespace ideam::core {

template <typename T, typename T_Strategy, size_t KernelSize>
struct alignas(64) StencilConvolutionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY_NUMERIC;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size = KernelSize;
    
    // Demand enough transient memory to build our SoA weights array for the hot loop
    static constexpr size_t transient_workspace_bytes = KernelSize * sizeof(T);

    // --- Configuration ---
    uint32_t grid_buffer_id = INVALID_ID;
    
    // Abstract spatial deltas [dx, dy, dz]. Aligns with Strategy dimensionality.
    std::array<std::array<int64_t, T_Strategy::dimensions>, KernelSize> kernel_deltas{};
    std::array<T, KernelSize> kernel_weights{};
    T center_weight = T{0};

    // --- Internal State Tracking ---
    uint32_t cached_buffer_version = 0;
    std::array<intptr_t, KernelSize> persistent_baked_offsets{};

    static godot::Array get_ui_properties() {
        godot::Array props;
        
        // 1. Center Weight (Type T)
        godot::Dictionary cw_prop;
        cw_prop["name"] = "center_weight";
        cw_prop["type"] = "T"; // Caught and parsed by your dynamic registry
        props.push_back(cw_prop);

        // 2. Kernel Weights (Fixed-size Array of T)
        godot::Dictionary kw_prop;
        kw_prop["name"] = "kernel_weights";
        kw_prop["type"] = godot::Variant::ARRAY;
        kw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        kw_prop["hint_string"] = "Array of T"; // Tells your UI builder what variants to lock the array to
        props.push_back(kw_prop);

        // 3. Kernel Deltas (Fixed-size Array of spatial offsets)
        godot::Dictionary kd_prop;
        kd_prop["name"] = "kernel_deltas";
        kd_prop["type"] = godot::Variant::ARRAY;
        kd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        kd_prop["hint_string"] = "Array of Int Offsets";
        props.push_back(kd_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return grid_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("center_weight")) {
            center_weight = static_cast<T>(p_props["center_weight"]);
        }
        if (p_props.has("kernel_weights")) {
            godot::Array kw_array = p_props["kernel_weights"];
            for (size_t i = 0; i < KernelSize && i < kw_array.size(); ++i) {
                kernel_weights[i] = static_cast<T>(kw_array[i]);
            }
        }
        if (p_props.has("kernel_deltas")) {
            godot::Array kd_array = p_props["kernel_deltas"];
            for (size_t i = 0; i < KernelSize && i < kd_array.size(); ++i) {
                godot::Array delta_pair = kd_array[i];
                for (size_t d = 0; d < T_Strategy::dimensions && d < delta_pair.size(); ++d) {
                    kernel_deltas[i][d] = static_cast<int64_t>(delta_pair[d]);
                }
            }
        }
    }
    
    /**
     * get_transient_requirement (Dynamic Scaling)
     * We just need space for our Kernel weights. 
     */
    inline size_t get_transient_requirement(const TaskContextPOD& p_context) const {
        return KernelSize * sizeof(T); 
    }

    /**
     * prepare (The Pre-Flight)
     * Executed before the graph barrier sync. Perfect time to handle offset math
     * and weaponize our transient memory to avoid cache thrashing.
     */
    inline void prepare(const TaskContextPOD& p_context) {
        const GrantPartPOD* part = p_context.get_grant_part(grid_buffer_id);
        if (!part) return;

        // 1. Selectively recalculate spatial offsets if the buffer layout changed
        if (part->buffer_version_at_issue != cached_buffer_version) {
            persistent_baked_offsets = DefaultView::bake_spatial_offsets(
                T_Strategy{}, part->element_stride, kernel_deltas
            );
            cached_buffer_version = part->buffer_version_at_issue;
        }

        // 2. Weaponize local_workspace (Aligning weights for SIMD)
        // Copy weights into the transient scratchpad to ensure they sit on a fresh, 
        // contiguous cache line right beside our execution frame.
        if (p_context.local_workspace) {
            T* fast_weights = new (p_context.local_workspace) T[KernelSize];
            for (size_t i = 0; i < KernelSize; ++i) {
                fast_weights[i] = kernel_weights[i];
            }
        }
    }

    /**
     * configure_view (The Binder)
     * Maps our pre-calculated state into the primary read view right before execution.
     */
    inline void configure_view(DefaultView& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const {
        p_view.baked_offsets = persistent_baked_offsets;
    }

    /**
     * execute_transform (The Hot Loop)
     */
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void execute_transform(const TaskContextPOD& context, T_View& main_view) const {
        const MemoryBufferSelectionPOD* sel = context.get_selection(grid_buffer_id);
        if (!sel || sel->mode != SelectionMode::DENSE) return;

        // 1. Spin up the Secondary SwapView for State T+1 Writes
        SwapView<T, T_Strategy> write_view;
        write_view.bind_secondary(context.get_grant_part(grid_buffer_id));

        // 2. The Graceful Fallback (Zero Branching inside the loop)
        // If transient memory was exhausted, context.local_workspace is null.
        // We gracefully degrade to the cold-storage weights inside the Logic struct.
        const T* active_weights = context.local_workspace ? 
            reinterpret_cast<const T*>(context.local_workspace) : 
            kernel_weights.data();
        
        const int64_t count = sel->capacity;
        const uint64_t* bitset = sel->data.bitset;

        // --- THE HOT LOOP ---
        for (int64_t i = 0; i < count; ++i) {
            if (!(bitset[i >> 6] & (1ULL << (i & 63)))) continue;

            main_view[i];
            
            T accumulator = main_view.center() * center_weight;

            // Prefetcher blasts through active_weights, oblivious to where it lives.
            for (size_t k = 0; k < KernelSize; ++k) {
                accumulator += main_view.neighbor(k) * active_weights[k];
            }

            write_view[i].write() = accumulator;
        }
    }
};

} // namespace ideam::core