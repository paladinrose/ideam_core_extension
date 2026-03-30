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

namespace ideam::core {

/**
 * FrustumCullLogic
 * Prunes elements that fall outside a defined set of clipping planes.
 * T: The position type (usually Vector3).
 * MAGIC: Uses internal SoA plane representation to maximize cache-line 
 * utilization during the 6-plane intersection test.
 */
template <typename T = godot::Vector3>
struct FrustumCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    // --- Configuration Data ---
    static constexpr uint32_t MAX_PLANES = 6;

    uint32_t column_id = 0;
    uint32_t num_planes = 0;
    
    // Internal SoA for Planes: Pre-calculated for zero-overhead dot products
    std::array<float, MAX_PLANES> nx;
    std::array<float, MAX_PLANES> ny;
    std::array<float, MAX_PLANES> nz;
    std::array<float, MAX_PLANES> d;

    /**
     * configure_planes
     * Convenience method for the UI/Task-Graph to set up the SoA data.
     */
    void configure_planes(const std::vector<godot::Plane>& p_planes) {
        num_planes = std::min(static_cast<uint32_t>(p_planes.size()), MAX_PLANES);
        for (uint32_t i = 0; i < num_planes; ++i) {
            nx[i] = p_planes[i].normal.x;
            ny[i] = p_planes[i].normal.y;
            nz[i] = p_planes[i].normal.z;
            d[i]  = p_planes[i].d;
        }
    }

    /**
     * execute_cull
     * Evaluates frustum containment and prunes the selection.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (num_planes == 0) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            _cull_sparse(r_selection, p_view);
        }
    }

private:
    /**
     * _evaluate
     * Core Frustum Test.
     * Point is "inside" if it is on the positive side of ALL planes.
     */
    inline bool _evaluate(const T& p_pos) const {
        const float px = p_pos.x;
        const float py = p_pos.y;
        const float pz = p_pos.z;

        for (uint32_t i = 0; i < num_planes; ++i) {
            // Plane Equation: Ax + By + Cz + D >= 0
            if ((nx[i] * px + ny[i] * py + nz[i] * pz) + d[i] < 0.0f) {
                return false;
            }
        }
        return true;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        for (int64_t i = 0; i < count; ++i) {
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
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            if (_evaluate(p_view[idx])) {
                indices[write_ptr++] = idx;
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_FRUSTUM_CULL_LOGIC_H