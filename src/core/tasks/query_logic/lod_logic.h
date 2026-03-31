#ifndef IDEAM_CORE_LOD_LOGIC_H
#define IDEAM_CORE_LOD_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <type_traits>
#include <cmath>
#include <array>

namespace ideam::core {

/**
 * LODLogic<T, N>
 * Maps element values (like distance to camera) to a Level of Detail (LOD) tier.
 * Updates the `lod_levels` shadow buffer transparently.
 * Compiles to a branchless single-check (N=1) or an unrolled multi-value mapping (N>1).
 */
template <typename T, size_t N = 1>
struct LODLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    struct Mapping {
        T target_value;
        uint8_t lod_level;
    };

    // --- Configuration ---
    std::array<Mapping, N> mappings;
    uint8_t default_lod = 0;
    
    // Optional tolerance for floating-point and vector clustering
    float tolerance = 0.0001f;

    template <typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!r_selection.lod_levels || r_selection.element_count == 0) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _dispatch_dense(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            _dispatch_sparse(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::RANGE) {
            _dispatch_range(r_selection, p_view);
        }
    }

private:
    [[nodiscard]] inline bool _matches(const T& p_val, const T& p_target) const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            return std::abs(p_val - p_target) <= tolerance;
        } 
        else if constexpr (requires { p_val.distance_squared_to(p_target); }) {
            return p_val.distance_squared_to(p_target) <= (tolerance * tolerance);
        } 
        else {
            return p_val == p_target;
        }
    }

    [[nodiscard]] inline uint8_t _get_lod(const T& p_val) const noexcept {
        if constexpr (N == 1) {
            // Branchless evaluation for atomic mappings
            return _matches(p_val, mappings[0].target_value) ? mappings[0].lod_level : default_lod;
        } else {
            // Compiler completely unrolls this loop
            for (size_t i = 0; i < N; ++i) {
                if (_matches(p_val, mappings[i].target_value)) {
                    return mappings[i].lod_level;
                }
            }
            return default_lod;
        }
    }

    // --- Core High-Speed Loops ---

    template <typename T_View>
    inline void _dispatch_dense(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        const uint64_t* bitset = r_sel.data.bitset;
        uint8_t* lods = r_sel.lod_levels;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                lods[i] = _get_lod(p_view[i]);
            }
        }
    }

    template <typename T_View>
    inline void _dispatch_sparse(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        const int64_t* indices = r_sel.data.indices;
        uint8_t* lods = r_sel.lod_levels;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            lods[idx] = _get_lod(p_view[idx]);
        }
    }

    template <typename T_View>
    inline void _dispatch_range(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        uint8_t* lods = r_sel.lod_levels;
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            lods[i] = _get_lod(p_view[i]);
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_LOD_LOGIC_H