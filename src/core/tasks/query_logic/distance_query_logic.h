#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <cmath>
#include <type_traits>
#include <bit>

namespace ideam::core {

template <typename T>
struct DistanceQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::VECTOR4 | DataType::VECTOR4I | DataType::VECTOR4D;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    enum class Comparison : uint8_t {
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL
    };

    uint32_t target_buffer_id = 0;
    uint32_t column_id   = 0; 
    T target_point;
    float distance_threshold = 0.0f;
    Comparison op = Comparison::LESS_EQUAL;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary col_prop;
        col_prop["name"] = "column_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Equal,Not Equal,Less,Less Equal,Greater,Greater Equal";
        props.push_back(op_prop);

        godot::Dictionary target_prop;
        target_prop["name"] = "target_point";
        target_prop["type"] = "T"; 
        target_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(target_prop);

        godot::Dictionary dist_prop;
        dist_prop["name"] = "distance_threshold";
        dist_prop["type"] = godot::Variant::FLOAT;
        dist_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dist_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("column_id")) {
            column_id = static_cast<uint32_t>(p_props["column_id"]);
        }
        if (p_props.has("op")) {
            op = static_cast<Comparison>(static_cast<uint8_t>(p_props["op"]));
        }
        if (p_props.has("target_point")) {
            target_point = static_cast<T>(p_props["target_point"]);
        }
        if (p_props.has("distance_threshold")) {
            distance_threshold = static_cast<float>(p_props["distance_threshold"]);
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

private:
    // --- The DOD View Adapter ---
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

    template <Comparison O>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_val) const {
        // MAGIC: Squared distance comparison to avoid sqrt() in the hot loop
        float dist_sq = p_val.distance_squared_to(target_point);
        float thresh_sq = distance_threshold * distance_threshold;

        if constexpr (O == Comparison::EQUAL)         return std::abs(dist_sq - thresh_sq) < 0.0001f;
        if constexpr (O == Comparison::NOT_EQUAL)     return std::abs(dist_sq - thresh_sq) >= 0.0001f;
        if constexpr (O == Comparison::LESS)          return dist_sq < thresh_sq;
        if constexpr (O == Comparison::LESS_EQUAL)    return dist_sq <= thresh_sq;
        if constexpr (O == Comparison::GREATER)       return dist_sq > thresh_sq;
        if constexpr (O == Comparison::GREATER_EQUAL) return dist_sq >= thresh_sq;
        return false;
    }

    template <Comparison O, typename T_View>
    void _loop_dense(uint64_t* bitset, int64_t capacity, const T_View& p_view, int64_t& r_count) const {
        for (int64_t i = 0; i < capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate<O>(_read_view(p_view, i))) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_count--;
                }
            }
        }
    }

    template <Comparison O, typename T_View>
    void _loop_sparse(int64_t* indices, int64_t count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < count; ++i) {
            if (_evaluate<O>(_read_view(p_view, indices[i]))) {
                indices[r_write_ptr++] = indices[i];
            }
        }
    }

    template <Comparison O, typename T_View>
    void _loop_add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate<O>(_read_view(p_view, global_index))) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        switch (op) {
            case Comparison::EQUAL:         _loop_dense<Comparison::EQUAL>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case Comparison::NOT_EQUAL:     _loop_dense<Comparison::NOT_EQUAL>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case Comparison::LESS:          _loop_dense<Comparison::LESS>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case Comparison::LESS_EQUAL:    _loop_dense<Comparison::LESS_EQUAL>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case Comparison::GREATER:       _loop_dense<Comparison::GREATER>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
            case Comparison::GREATER_EQUAL: _loop_dense<Comparison::GREATER_EQUAL>(bitset, r_selection.capacity, p_view, r_selection.element_count); break;
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        switch (op) {
            case Comparison::EQUAL:         _loop_sparse<Comparison::EQUAL>(indices, r_selection.element_count, p_view, write_ptr); break;
            case Comparison::NOT_EQUAL:     _loop_sparse<Comparison::NOT_EQUAL>(indices, r_selection.element_count, p_view, write_ptr); break;
            case Comparison::LESS:          _loop_sparse<Comparison::LESS>(indices, r_selection.element_count, p_view, write_ptr); break;
            case Comparison::LESS_EQUAL:    _loop_sparse<Comparison::LESS_EQUAL>(indices, r_selection.element_count, p_view, write_ptr); break;
            case Comparison::GREATER:       _loop_sparse<Comparison::GREATER>(indices, r_selection.element_count, p_view, write_ptr); break;
            case Comparison::GREATER_EQUAL: _loop_sparse<Comparison::GREATER_EQUAL>(indices, r_selection.element_count, p_view, write_ptr); break;
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        switch (op) {
            case Comparison::EQUAL:         _loop_add_available<Comparison::EQUAL>(r_selection, p_view, p_ctx); break;
            case Comparison::NOT_EQUAL:     _loop_add_available<Comparison::NOT_EQUAL>(r_selection, p_view, p_ctx); break;
            case Comparison::LESS:          _loop_add_available<Comparison::LESS>(r_selection, p_view, p_ctx); break;
            case Comparison::LESS_EQUAL:    _loop_add_available<Comparison::LESS_EQUAL>(r_selection, p_view, p_ctx); break;
            case Comparison::GREATER:       _loop_add_available<Comparison::GREATER>(r_selection, p_view, p_ctx); break;
            case Comparison::GREATER_EQUAL: _loop_add_available<Comparison::GREATER_EQUAL>(r_selection, p_view, p_ctx); break;
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_DISTANCE_QUERY_LOGIC_H