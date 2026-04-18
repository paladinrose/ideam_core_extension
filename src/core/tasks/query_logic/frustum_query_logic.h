#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <array>
#include <bit>

namespace ideam::core {

template <typename T = godot::Vector3>
struct FrustumQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY_VECTOR3;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr uint32_t MAX_PLANES = 6;

    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;
    uint32_t num_planes = 0;

    // SoA structure for 6 planes to maximize L1 cache usage
    std::array<godot::Vector3, MAX_PLANES> plane_normals;
    std::array<float, MAX_PLANES> plane_distances;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

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
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_pos) const {
        for (uint32_t p = 0; p < num_planes; ++p) {
            if (plane_normals[p].dot(p_pos) - plane_distances[p] > 0.0f) {
                return false; // Outside frustum
            }
        }
        return true; 
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(p_view[i])) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t write_ptr = 0;
        int64_t* indices = r_selection.data.indices;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (_evaluate(p_view[indices[i]])) {
                indices[write_ptr++] = indices[i];
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
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

                if (_evaluate(p_view[global_index])) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_FRUSTUM_QUERY_LOGIC_H