#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <godot_cpp/variant/color.hpp>
#include <bit>

namespace ideam::core {

template <typename T>
struct ColorRGBAQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::COLOR;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;
    
    enum class RgbaMode : uint8_t {
        CHANNELS,
        DISTANCE
    };

    enum class Comparison : uint8_t { 
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL 
    };

    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;
    
    RgbaMode mode = RgbaMode::CHANNELS;
    Comparison op = Comparison::EQUAL;
    uint32_t mask = 15; // Bitmask: R=1, G=2, B=4, A=8
    
    float threshold = 0.5f;        // Used for channel comparison
    godot::Color target_color;     // Used as the origin point for DISTANCE mode
    float distance_threshold_sq = 0.25f; // Pre-calculated: threshold * threshold

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Channels,Distance";
        props.push_back(mode_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Equal,Not Equal,Less,Less Equal,Greater,Greater Equal";
        props.push_back(op_prop);

        godot::Dictionary mask_prop;
        mask_prop["name"] = "mask";
        mask_prop["type"] = godot::Variant::INT;
        mask_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        mask_prop["hint_string"] = "Red,Green,Blue,Alpha";
        props.push_back(mask_prop);

        godot::Dictionary thresh_prop;
        thresh_prop["name"] = "threshold";
        thresh_prop["type"] = godot::Variant::FLOAT;
        thresh_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(thresh_prop);

        godot::Dictionary color_prop;
        color_prop["name"] = "target_color";
        color_prop["type"] = godot::Variant::COLOR;
        color_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(color_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("mode")) {
            mode = static_cast<RgbaMode>(static_cast<uint8_t>(p_props["mode"]));
        }
        if (p_props.has("op")) {
            op = static_cast<Comparison>(static_cast<uint8_t>(p_props["op"]));
        }
        if (p_props.has("mask")) {
            mask = static_cast<uint32_t>(p_props["mask"]);
        }
        if (p_props.has("threshold")) {
            threshold = static_cast<float>(p_props["threshold"]);
            distance_threshold_sq = threshold * threshold; // Update pre-calculated value
        }
        if (p_props.has("target_color")) {
            target_color = static_cast<godot::Color>(p_props["target_color"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        // Template dispatching: Hoist the comparison operator out of the tight loop
        switch (op) {
            case Comparison::EQUAL:         _execute_impl<Op, Comparison::EQUAL>(r_selection, p_context, p_view); break;
            case Comparison::NOT_EQUAL:     _execute_impl<Op, Comparison::NOT_EQUAL>(r_selection, p_context, p_view); break;
            case Comparison::LESS:          _execute_impl<Op, Comparison::LESS>(r_selection, p_context, p_view); break;
            case Comparison::LESS_EQUAL:    _execute_impl<Op, Comparison::LESS_EQUAL>(r_selection, p_context, p_view); break;
            case Comparison::GREATER:       _execute_impl<Op, Comparison::GREATER>(r_selection, p_context, p_view); break;
            case Comparison::GREATER_EQUAL: _execute_impl<Op, Comparison::GREATER_EQUAL>(r_selection, p_context, p_view); break;
        }
    }

private:
    template <QueryOp Op, Comparison Cmp, typename T_View>
    void _execute_impl(MemoryBufferSelectionPOD& r_selection, 
                       const TaskContextPOD& p_context, 
                       const T_View& p_view) const {
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense<Cmp>(r_selection, p_view);
            else _cull_sparse<Cmp>(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available<Cmp>(r_selection, p_view, p_context);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T>(p_view[idx]); }) {
            return static_cast<T>(p_view[idx]);
        } else {
            return T{}; 
        }
    }

    template <Comparison Cmp>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _compare(float p_val, float p_thresh) const {
        if constexpr (Cmp == Comparison::EQUAL)         return p_val == p_thresh;
        if constexpr (Cmp == Comparison::NOT_EQUAL)     return p_val != p_thresh;
        if constexpr (Cmp == Comparison::LESS)          return p_val < p_thresh;
        if constexpr (Cmp == Comparison::LESS_EQUAL)    return p_val <= p_thresh;
        if constexpr (Cmp == Comparison::GREATER)       return p_val > p_thresh;
        if constexpr (Cmp == Comparison::GREATER_EQUAL) return p_val >= p_thresh;
        return false;
    }

    template <Comparison Cmp>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_val) const {
        if (mode == RgbaMode::DISTANCE) {
            const float dr = p_val.r - target_color.r;
            const float dg = p_val.g - target_color.g;
            const float db = p_val.b - target_color.b;
            return _compare<Cmp>(dr * dr + dg * dg + db * db, distance_threshold_sq);
        }

        // CHANNELS
        if ((mask & 1) && !_compare<Cmp>(p_val.r, threshold)) return false;
        if ((mask & 2) && !_compare<Cmp>(p_val.g, threshold)) return false;
        if ((mask & 4) && !_compare<Cmp>(p_val.b, threshold)) return false;
        if ((mask & 8) && !_compare<Cmp>(p_val.a, threshold)) return false;
        
        return true;
    }

    template <Comparison Cmp, typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<Cmp>(_read_view(p_view, i))) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <Comparison Cmp, typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (_evaluate<Cmp>(_read_view(p_view, indices[i]))) {
                indices[write_ptr++] = indices[i];
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <Comparison Cmp, typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask_chunk = unclaimed[w];
            while (mask_chunk != 0) {
                int bit_index = std::countr_zero(mask_chunk);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate<Cmp>(_read_view(p_view, global_index))) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask_chunk &= (mask_chunk - 1); 
            }
        }
    }
};

} // namespace ideam::core