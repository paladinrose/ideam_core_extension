#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h" 
#include "query_logic_traits.h"
#include <godot_cpp/variant/vector3.hpp>
#include <bit>

namespace ideam::core {

/* TODO:
- Add support for Vector3I and Vector3D in get_ui_properties() and apply_properties(), ensuring proper type handling in the evaluation logic.
*/
/**
 * AABBQueryLogic
 * Performs a spatial intersection test between a 3D Box and the target buffer.
 * CULL: Prunes non-intersecting elements in-place.
 * ADD: Scans the Availability Mask and queues unclaimed intersecting elements.
 */
struct AABBQueryLogic {
    using ValueType       = godot::Vector3; 
    using DefaultStrategy = Spatial3DStrategy;
    using DefaultView     = SingleElementView<ValueType, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    // Upgraded to allow standard 1D contiguous vectors to be queried spatially
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL; 
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;
    
    // UI/Compiler Routing
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    godot::Vector3 box_min;
    godot::Vector3 box_max;
    uint32_t target_buffer_id = 0; 

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary min_prop;
        min_prop["name"] = "box_min";
        min_prop["type"] = godot::Variant::VECTOR3;
        min_prop["hint"] = godot::PROPERTY_HINT_NONE;
        min_prop["hint_string"] = "suffix:m";
        props.push_back(min_prop);

        godot::Dictionary max_prop;
        max_prop["name"] = "box_max";
        max_prop["type"] = godot::Variant::VECTOR3;
        max_prop["hint"] = godot::PROPERTY_HINT_NONE;
        max_prop["hint_string"] = "suffix:m";
        props.push_back(max_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("box_min")) {
            box_min = static_cast<godot::Vector3>(p_props["box_min"]);
        }
        if (p_props.has("box_max")) {
            box_max = static_cast<godot::Vector3>(p_props["box_max"]);
        }
    }
    
    template<QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            const int64_t count = r_selection.element_count;
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view, count);
            else _cull_indexed(r_selection, p_view, count);
        } else if constexpr (Op == QueryOp::ADD) {
            // Addition evaluates the Availability Mask natively, bypassing mode checks.
            _add_available(r_selection, p_view, p_context);
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline auto _read_view(const T_View& p_view, int64_t idx) const {
        using RawType = std::remove_pointer_t<decltype(p_view[idx])>;
        using DecayedType = std::decay_t<RawType>;
        
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const DecayedType*>(p_view[idx]);
        } else {
            return static_cast<DecayedType>(p_view[idx]);
        }
    }

    template<typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, int64_t p_count) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(_read_view(p_view, i))) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template<typename T_View>
    void _cull_indexed(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, int64_t p_count) const {
        if (r_selection.mode == SelectionMode::SPARSE) {
            int64_t write_ptr = 0;
            for (int64_t i = 0; i < p_count; ++i) {
                if (_evaluate(_read_view(p_view, r_selection.data.indices[i]))) {
                    r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
                }
            }
            r_selection.element_count = write_ptr;
        } 
    }

    template<typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate(_read_view(p_view, global_index))) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                
                mask &= (mask - 1); 
            }
        }
    }

    template<typename T>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_val) const {
        if constexpr (std::is_same_v<T, godot::Vector2> || std::is_same_v<T, godot::Vector2i>) {
            return (p_val.x >= box_min.x && p_val.x <= box_max.x) &&
                   (p_val.y >= box_min.y && p_val.y <= box_max.y);
        } else {
            return (p_val.x >= box_min.x && p_val.x <= box_max.x) &&
                   (p_val.y >= box_min.y && p_val.y <= box_max.y) &&
                   (p_val.z >= box_min.z && p_val.z <= box_max.z);
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_AABB_QUERY_LOGIC_H