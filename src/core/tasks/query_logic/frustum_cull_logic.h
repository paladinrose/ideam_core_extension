#ifndef IDEAM_CORE_FRUSTUM_CULL_LOGIC_H
#define IDEAM_CORE_FRUSTUM_CULL_LOGIC_H

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
struct FrustumCullLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr uint32_t MAX_PLANES = 6;

    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;
    uint32_t num_planes = 0;

    // SoA structure for 6 planes to maximize L1 cache usage in the hot loop
    std::array<float, MAX_PLANES> nx;
    std::array<float, MAX_PLANES> ny;
    std::array<float, MAX_PLANES> nz;
    std::array<float, MAX_PLANES> d;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void set_planes(const std::vector<godot::Plane>& p_planes) {
        num_planes = std::min(static_cast<uint32_t>(p_planes.size()), MAX_PLANES);
        for (uint32_t i = 0; i < num_planes; ++i) {
            nx[i] = p_planes[i].normal.x;
            ny[i] = p_planes[i].normal.y;
            nz[i] = p_planes[i].normal.z;
            d[i]  = p_planes[i].d;
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_pos) const {
        const float px = p_pos.x;
        const float py = p_pos.y;
        const float pz = p_pos.z;

        for (uint32_t i = 0; i < num_planes; ++i) {
            if ((nx[i] * px + ny[i] * py + nz[i] * pz) + d[i] < 0.0f) {
                return false;
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

#endif // IDEAM_CORE_FRUSTUM_CULL_LOGIC_H