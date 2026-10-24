#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/swap_view.h" 
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <array>
#include <new> 

namespace ideam::core {

/**
 * StencilConvolutionTransformLogic<T, Strategy, KernelSize>
 * Applies a fixed-topology MAC (Multiply-Accumulate) convolution over a spatial grid.
 * Examples: Gaussian Blur, Sobel Edge Detection, Cellular Automata.
 * EXCLUSIVELY STATIC: Enforces compile-time unrolling of the MAC sequence.
 */
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

    static constexpr std::string_view display_name = "Stencil Convolution";
    
    // --- Configuration ---
    uint32_t grid_buffer_id = INVALID_ID;
    int64_t buffer_width = 0;
    uint32_t output_buffer_id = INVALID_ID;

    // Abstract spatial deltas [dx, dy, dz]. Aligns with Strategy dimensionality.
    std::array<std::array<int64_t, T_Strategy::dimensions>, KernelSize> kernel_deltas{};
    std::array<T, KernelSize> kernel_weights{};
    T center_weight = T{};

    // --- Internal State Tracking ---
    uint32_t cached_buffer_version = 0;
    std::array<intptr_t, KernelSize> persistent_baked_offsets{};

    static godot::Array get_ui_properties() {
        godot::Array props;
        
        godot::Dictionary cw_prop;
        cw_prop["name"] = "center_weight";
        cw_prop["type"] = "T"; 
        props.push_back(cw_prop);

        godot::Dictionary kernel_size_prop;
        kernel_size_prop["name"] = "kernel_size";
        kernel_size_prop["type"] = godot::Variant::INT;
        kernel_size_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        kernel_size_prop["hint_string"] = "logic_options";
        props.push_back(kernel_size_prop);
        
        godot::Dictionary kw_prop;
        kw_prop["name"] = "kernel_weights";
        kw_prop["type"] = godot::Variant::ARRAY;
        kw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        kw_prop["hint_string"] = "Array of T"; 
        props.push_back(kw_prop);

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
        if constexpr (!std::is_same_v<T, void*>) {
            if (p_props.has("center_weight")) {
                center_weight = static_cast<T>(p_props["center_weight"]);
            }
            if (p_props.has("kernel_weights")) {
                godot::Array kw_array = p_props["kernel_weights"];
                for (size_t i = 0; i < KernelSize && i < kw_array.size(); ++i) {
                    kernel_weights[i] = static_cast<T>(kw_array[i]);
                }
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
        if(p_props.has("grid_buffer_id")) {
            grid_buffer_id = static_cast<uint32_t>(p_props["grid_buffer_id"]);
        }
        if(p_props.has("output_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(p_props["output_buffer_id"]);
        }
        if(p_props.has("buffer_width")) {
            buffer_width = static_cast<int64_t>(p_props["buffer_width"]);
        }
    }
    
    [[nodiscard]] inline size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        MemoryBufferSelectionPOD* sel = p_context.get_selection(grid_buffer_id);
        return MemoryUtilities::align_to(KernelSize * sizeof(T), 64) + (sel ? sel->capacity * sizeof(T) : 0);
    }

    inline void prepare(const TaskContextPOD& p_context) {
        const GrantPartPOD* part = p_context.get_grant_part(grid_buffer_id);
        if (!part) return;

        // 1. Selectively recalculate spatial byte-offsets if the buffer layout changed
        if (part->buffer_version_at_issue != cached_buffer_version) {
            T_Strategy strat{};
            const uint32_t effective_stride = (part->element_stride > 0)
                ? part->element_stride
                : static_cast<uint32_t>(sizeof(T));

            int64_t w = buffer_width;
            if (w <= 0 && part->selection.capacity > 0) {
                w = static_cast<int64_t>(std::sqrt(static_cast<double>(part->selection.capacity)));
            }

            if constexpr (requires { strat.stride_y; }) {
                strat.stride_y = w * static_cast<int64_t>(effective_stride);
            }

            persistent_baked_offsets = DefaultView::bake_spatial_offsets(
                strat, effective_stride, kernel_deltas
            );
            cached_buffer_version = part->buffer_version_at_issue;
        }

        // 2. Weaponize local_workspace (Aligning weights for L1 Cache)
        if (p_context.local_workspace) {
            T* fast_weights = new (p_context.local_workspace) T[KernelSize];
            for (size_t i = 0; i < KernelSize; ++i) {
                fast_weights[i] = kernel_weights[i];
            }
        }
    }

    inline void configure_view(DefaultView& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const {
        p_view.baked_offsets = persistent_baked_offsets;
    }

    template <typename T_View, typename T_StrategyType>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void execute(const TaskContextPOD& context, const T_View& main_view) const {
        const GrantPartPOD* part = context.get_grant_part(grid_buffer_id);
        const MemoryBufferSelectionPOD* sel = context.get_selection(grid_buffer_id);
        if (!part || !part->raw_base_ptr || !sel || sel->mode != SelectionMode::DENSE) return;

        const int64_t count = sel->capacity;
        if (count <= 0) return;
        const uint64_t* bitset = sel->data.bitset;
        const intptr_t stride = static_cast<intptr_t>(part->element_stride > 0 ? part->element_stride : sizeof(T));

        T* dst_ptr = reinterpret_cast<T*>(part->raw_base_ptr);
        const T* src_ptr = dst_ptr;

        if (output_buffer_id != INVALID_ID && output_buffer_id != grid_buffer_id) {
            if (const GrantPartPOD* out_part = context.get_grant_part(output_buffer_id)) {
                if (out_part->raw_base_ptr) {
                    dst_ptr = reinterpret_cast<T*>(out_part->raw_base_ptr);
                }
            }
        } else if (context.local_workspace) {
            // Snapshot input after the kernel weights in transient workspace to prevent in-place read-after-write corruption.
            T* snapshot = reinterpret_cast<T*>(static_cast<uint8_t*>(context.local_workspace) + MemoryUtilities::align_to(KernelSize * sizeof(T), 64));
            if constexpr (std::is_arithmetic_v<T>) {
                // Normalize selected cells to 1 and unselected (culled) cells to 0 so multi-step cellular automata
                // convolve clean binary states even when dst_ptr stores encoded sums (e.g., 3, 11, 12) for palette LUTs.
                for (int64_t i = 0; i < count; ++i) {
                    const bool is_active = (bitset[i >> 6] & (1ULL << (i & 63))) != 0;
                    snapshot[i] = (is_active && dst_ptr[i] != T{}) ? static_cast<T>(1) : T{};
                }
            } else {
                std::memcpy(snapshot, dst_ptr, static_cast<size_t>(count) * sizeof(T));
            }
            src_ptr = snapshot;
        }

        const T* active_weights = context.local_workspace ?
            reinterpret_cast<const T*>(context.local_workspace) :
            kernel_weights.data();

        for (int64_t i = 0; i < count; ++i) {
            if (!(bitset[i >> 6] & (1ULL << (i & 63)))) {
                dst_ptr[i] = T{};
                continue;
            }

            T accumulator = src_ptr[i] * center_weight;

            for (size_t k = 0; k < KernelSize; ++k) {
                const int64_t elem_offset = static_cast<int64_t>(persistent_baked_offsets[k] / stride);
                const int64_t neighbor_idx = ((i + elem_offset) % count + count) % count;
                accumulator += src_ptr[neighbor_idx] * active_weights[k];
            }

            dst_ptr[i] = accumulator;
        }
    }
};

} // namespace ideam::core