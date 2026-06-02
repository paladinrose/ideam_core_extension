#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

template <typename T>
struct ColorHSVAQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::COLOR; // Or custom HSVA type identifier
    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;
    
    enum class Comparison : uint8_t { 
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL 
    };

    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;
    
    Comparison op = Comparison::EQUAL;
    uint32_t mask = 15; // Bitmask: H=1, S=2, V=4, A=8
    float threshold = 0.5f;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary op_prop;
        op_prop["name"] = "op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Equal,Not Equal,Less,Less Equal,Greater,Greater Equal";
        props.push_back(op_prop);

        godot::Dictionary mask_prop;
        mask_prop["name"] = "mask";
        mask_prop["type"] = godot::Variant::INT;
        mask_prop["hint"] = godot::PROPERTY_HINT_FLAGS; // Suggests a checkbox list in UI
        mask_prop["hint_string"] = "Hue,Saturation,Value,Alpha";
        props.push_back(mask_prop);

        godot::Dictionary thresh_prop;
        thresh_prop["name"] = "threshold";
        thresh_prop["type"] = godot::Variant::FLOAT;
        thresh_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(thresh_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("op")) {
            op = static_cast<Comparison>(static_cast<uint8_t>(p_props["op"]));
        }
        if (p_props.has("mask")) {
            mask = p_props["mask"];
        }
        if (p_props.has("threshold")) {
            threshold = p_props["threshold"];
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        // Template dispatching ensures branchless execution in the internal loops
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
        // T is assumed to map (r, g, b, a) to (H, S, V, A) directly from memory
        if ((mask & 1) && !_compare<Cmp>(p_val.r, threshold)) return false; // Hue
        if ((mask & 2) && !_compare<Cmp>(p_val.g, threshold)) return false; // Saturation
        if ((mask & 4) && !_compare<Cmp>(p_val.b, threshold)) return false; // Value
        if ((mask & 8) && !_compare<Cmp>(p_val.a, threshold)) return false; // Alpha
        
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