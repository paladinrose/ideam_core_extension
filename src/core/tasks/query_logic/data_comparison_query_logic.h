#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>

namespace ideam::core {

template <typename T>
struct DataComparisonQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::BYTE | DataType::INT32 | DataType::INT64 | DataType::FLOAT32 | DataType::FLOAT64;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;
    
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    enum class Operator : uint8_t { EQUAL, NOT_EQUAL, LESS_THAN, LESS_EQUAL, GREATER_THAN, GREATER_EQUAL };

    uint32_t target_buffer_id = 0;
    uint32_t column_id_a = 0;
    
    // Config for the secondary buffer
    uint32_t comparison_buffer_id = 0;
    uint32_t column_id_b = 0;
    Operator op = Operator::EQUAL;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary op_prop;
        op_prop["name"] = "op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Equal,Not Equal,Less Than,Less Equal,Greater Than,Greater Equal";
        props.push_back(op_prop);

        godot::Dictionary col_a_prop;
        col_a_prop["name"] = "column_id_a";
        col_a_prop["type"] = godot::Variant::INT;
        col_a_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_a_prop);

        godot::Dictionary comparison_buffer_prop;
        comparison_buffer_prop["name"] = "comparison_buffer_id";
        comparison_buffer_prop["type"] = godot::Variant::INT;
        comparison_buffer_prop["hint_string"] = "buffer_option";
        props.push_back(comparison_buffer_prop);

        godot::Dictionary col_b_prop;
        col_b_prop["name"] = "column_id_b";
        col_b_prop["type"] = godot::Variant::INT;
        col_b_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_b_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("op")) {
            op = static_cast<Operator>(static_cast<uint8_t>(p_props["op"]));
        }
        if (p_props.has("column_id_a")) {
            column_id_a = static_cast<uint32_t>(p_props["column_id_a"]);
        }
        if (p_props.has("comparison_buffer_id")) {
            comparison_buffer_id = static_cast<uint32_t>(p_props["comparison_buffer_id"]);
        }
        if (p_props.has("column_id_b")) {
            column_id_b = static_cast<uint32_t>(p_props["column_id_b"]);
        }
    }
    
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
    
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& a, const T& b) const noexcept {
        switch (op) {
            case Operator::EQUAL:         return a == b;
            case Operator::NOT_EQUAL:     return a != b;
            case Operator::LESS_THAN:     return a < b;
            case Operator::LESS_EQUAL:    return a <= b;
            case Operator::GREATER_THAN:  return a > b;
            case Operator::GREATER_EQUAL: return a >= b;
            default:                      return false;
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void execute(MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_context, const T_View& p_view) const {
        
        // 1. Fetch the secondary buffer. NO VIEW CONSTRUCTION. 
        const GrantPartPOD* part_b = p_context.get_grant_part(comparison_buffer_id);
        if (!part_b) return;

        // 2. Map to a restricted raw pointer. 
        // __restrict guarantees the optimizer that writing to r_selection won't overlap with buffer_b.
        const T* __restrict buffer_b = reinterpret_cast<const T*>(part_b->raw_base_ptr);

        if constexpr (Op == QueryOp::CULL) {
            
            // --- HIGH PERFORMANCE DENSE CULL ---
            if (r_selection.mode == SelectionMode::DENSE) {
                uint64_t* bitset = r_selection.data.bitset;
                const int64_t words = (r_selection.capacity + 63) >> 6;
                
                for (int64_t w = 0; w < words; ++w) {
                    uint64_t mask = bitset[w];
                    while (mask != 0) {
                        int bit_index = std::countr_zero(mask);
                        int64_t global_index = (w << 6) + bit_index;
                        
                        if (global_index >= r_selection.capacity) break;

                        // Compare primary (View Lens) to secondary (Raw Pointer)
                        if (_evaluate(_read_view(p_view, global_index), buffer_b[global_index])) {
                            bitset[w] &= ~(1ULL << bit_index);
                            r_selection.element_count--;
                        }
                        
                        mask &= mask - 1; // Clear lowest set bit
                    }
                }
            } 
            // --- HIGH PERFORMANCE SPARSE CULL ---
            else if (r_selection.mode == SelectionMode::SPARSE) {
                int64_t write_ptr = 0;
                int64_t* indices = r_selection.data.indices;
                
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    int64_t global_index = indices[i];
                    if (_evaluate(_read_view(p_view, global_index), buffer_b[global_index])) {
                        indices[write_ptr++] = global_index;
                    }
                }
                r_selection.element_count = write_ptr;
            }
            
        } 
        else if constexpr (Op == QueryOp::ADD) {
            
            // --- HIGH PERFORMANCE ADD ---
            const uint64_t* unclaimed = r_selection.unclaimed_mask;
            if (!unclaimed) return;

            const int64_t words = (r_selection.capacity + 63) >> 6;
            for (int64_t w = 0; w < words; ++w) {
                uint64_t mask = unclaimed[w];
                while (mask != 0) {
                    int bit_index = std::countr_zero(mask);
                    int64_t global_index = (w << 6) + bit_index;
                    
                    if (global_index >= r_selection.capacity) break;

                    if (_evaluate(_read_view(p_view, global_index), buffer_b[global_index])) {
                        if (r_selection.mode == SelectionMode::DENSE) {
                            r_selection.data.bitset[w] |= (1ULL << bit_index);
                            r_selection.element_count++;
                        }
                    }
                    mask &= mask - 1;
                }
            }
        }
    }
};

} // namespace ideam::core