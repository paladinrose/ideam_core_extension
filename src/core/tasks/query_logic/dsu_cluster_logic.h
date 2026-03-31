#ifndef IDEAM_CORE_DSU_CLUSTER_LOGIC_H
#define IDEAM_CORE_DSU_CLUSTER_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <vector>
#include <cmath>
#include <unordered_map>

namespace ideam::core {

enum class ClusterCompareMode : uint8_t {
    ABSOLUTE_VALUE,
    ANGULAR,
    BITMASK_MATCH
};

/**
 * DSUClusterLogic<T, PointCount>
 * Clusters adjacent elements into partitions using a Disjoint Set Union algorithm.
 * Fully delegates spatial topology to the provided StaticStencilView.
 */
template <typename T, size_t PointCount>
struct DSUClusterLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, PointCount>;

    static constexpr LogicRequirement requirements = LogicRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    // --- Configuration ---
    ClusterCompareMode mode = ClusterCompareMode::ABSOLUTE_VALUE;
    
    float tolerance = 0.1f;
    float cos_threshold = 1.0f;
    uint32_t bitmask = 0xFFFFFFFF;

    int64_t min_cluster_size = 1;
    int32_t partition_id_offset = 0;

    template <typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!r_selection.partition_ids || r_selection.element_count == 0) return;

        // 1. Build Dense mapping (Global Stride ID -> Local DSU Array Index)
        const int64_t total_capacity = r_selection.capacity;
        std::vector<int32_t> global_to_local(total_capacity, -1);
        std::vector<int64_t> local_to_global;
        local_to_global.reserve(r_selection.element_count);

        _build_mappings(r_selection, global_to_local, local_to_global);
        
        const int32_t n = static_cast<int32_t>(local_to_global.size());
        if (n == 0) return;

        // Extract stride to allow O(1) Pointer-to-Flat-Index conversion
        const GrantPartPOD* part = p_context.get_grant_part(r_selection.target_buffer_id);
        const intptr_t stride = part->element_stride;

        // 2. Initialize DSU Map
        std::vector<int32_t> dsu_map(n);
        for (int32_t i = 0; i < n; ++i) dsu_map[i] = i;

        // 3. Unify Neighbors
        for (int32_t i = 0; i < n; ++i) {
            const int64_t g_idx = local_to_global[i];
            
            // Because StaticStencilView's operator[] strictly requires N-D coords if Strategy::is_spatial,
            // we bypass it and legally mutate the center_ptr via our 1D index using C++26 mutable semantics.
            p_view.center_ptr = reinterpret_cast<uint8_t*>(p_view.head_ptr) + (g_idx * stride);
            const T& val_a = p_view.center();

            // Iterate exclusively over the View's pre-baked topology
            for (size_t p = 0; p < PointCount; ++p) {
                const T& neigh_val = p_view.neighbor(p);
                
                // O(1) Spatial fold: Pointer Math -> Flat Index
                const intptr_t byte_diff = reinterpret_cast<const uint8_t*>(&neigh_val) - reinterpret_cast<const uint8_t*>(p_view.head_ptr);
                const int64_t neigh_g_idx = byte_diff / stride;
                
                // Safety: Ensure neighbor didn't wrap out of bounds, and is in the active selection
                if (neigh_g_idx >= 0 && neigh_g_idx < total_capacity && global_to_local[neigh_g_idx] != -1) {
                    const int32_t local_neigh = global_to_local[neigh_g_idx];
                    
                    // Prevent redundant backward checks
                    if (local_neigh > i && _evaluate(val_a, neigh_val)) {
                        _unite(dsu_map, i, local_neigh);
                    }
                }
            }
        }

        // 4. Calculate cluster sizes & 5. Assign Partitions (Unchanged from previous iteration)
        std::vector<int32_t> root_sizes(n, 0);
        for (int32_t i = 0; i < n; ++i) root_sizes[_find_root(dsu_map, i)]++;

        std::unordered_map<int32_t, int32_t> root_to_part;
        int32_t current_p_id = partition_id_offset;

        for (int32_t i = 0; i < n; ++i) {
            const int64_t g_idx = local_to_global[i];
            const int32_t root = _find_root(dsu_map, i);

            if (root_sizes[root] < min_cluster_size) {
                _cull_element(r_selection, g_idx);
            } else {
                auto it = root_to_part.find(root);
                if (it == root_to_part.end()) {
                    it = root_to_part.insert({root, current_p_id++}).first;
                }
                r_selection.partition_ids[g_idx] = it->second;
            }
        }
        
        if (r_selection.mode == SelectionMode::SPARSE) _repack_sparse(r_selection);
    }

private:
    // ... [Inline Helpers _find_root, _unite, _evaluate, _build_mappings, _cull_element, _repack_sparse remain exactly the same as before]
};

} // namespace ideam::core

#endif // IDEAM_CORE_DSU_CLUSTER_LOGIC_H