#ifndef IDEAM_CORE_DSU_CLUSTER_METADATA_LOGIC_H
#define IDEAM_CORE_DSU_CLUSTER_METADATA_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <cmath>
#include <unordered_map>

namespace ideam::core {

enum class ClusterCompareMode : uint8_t {
    ABSOLUTE_VALUE,
    ANGULAR,
    BITMASK_MATCH
};

/**
 * DSUClusterMetadataLogic<T, PointCount>
 * Clusters adjacent elements into partitions using a Disjoint Set Union algorithm.
 * Fully delegates spatial topology to the provided StaticStencilView.
 * TRANSIENT MEMORY: Requires `capacity * 20` bytes for O(N) mapping arrays.
 */
template <typename T, size_t PointCount>
struct DSUClusterMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, PointCount>;

    static constexpr MetadataRequirement requirements = MetadataRequirement::REQUIRES_SPATIAL;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY_NUMERIC | DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3;
    static constexpr size_t transient_workspace_bytes = 0; // User must set via Graph to `capacity * 20`

    // --- Configuration ---
    ClusterCompareMode mode = ClusterCompareMode::ABSOLUTE_VALUE;
    
    float tolerance = 0.1f;
    float cos_threshold = 1.0f;
    uint32_t bitmask = 0xFFFFFFFF;

    int64_t min_cluster_size = 1;
    int32_t partition_id_offset = 0;
    uint32_t target_buffer_id = 0;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <typename T_View, typename T_Strategy>
    void execute_metadata(MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!r_selection.partition_ids || r_selection.element_count == 0 || !p_context.local_workspace) return;

        const int64_t total_capacity = r_selection.capacity;
        
        // 1. Transient Arena Allocations (Replaces std::vector)
        int32_t* global_to_local = static_cast<int32_t*>(p_context.local_workspace);
        int64_t* local_to_global = reinterpret_cast<int64_t*>(global_to_local + total_capacity);
        
        for (int64_t i = 0; i < total_capacity; ++i) global_to_local[i] = -1;

        int32_t n = 0;
        if (r_selection.mode == SelectionMode::DENSE) {
            const uint64_t* bitset = r_selection.data.bitset;
            for (int64_t i = 0; i < total_capacity; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    global_to_local[i] = n;
                    local_to_global[n++] = i;
                }
            }
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            for (int64_t i = 0; i < r_selection.element_count; ++i) {
                int64_t idx = r_selection.data.indices[i];
                global_to_local[idx] = n;
                local_to_global[n++] = idx;
            }
        }

        if (n == 0) return;

        // More transient arrays mapped right after local_to_global
        int32_t* dsu_map = reinterpret_cast<int32_t*>(local_to_global + total_capacity);
        int32_t* root_sizes = dsu_map + total_capacity;

        // 2. Initialize DSU Map
        for (int32_t i = 0; i < n; ++i) {
            dsu_map[i] = i;
            root_sizes[i] = 0;
        }

        const GrantPartPOD* part = p_context.get_grant_part(r_selection.target_buffer_id);
        const intptr_t stride = part->element_stride;

        // 3. Unify Neighbors
        for (int32_t i = 0; i < n; ++i) {
            const int64_t g_idx = local_to_global[i];
            
            p_view.center_ptr = reinterpret_cast<uint8_t*>(p_view.head_ptr) + (g_idx * stride);
            const T& val_a = p_view.center();

            for (size_t p = 0; p < PointCount; ++p) {
                const T& neigh_val = p_view.neighbor(p);
                const intptr_t byte_diff = reinterpret_cast<const uint8_t*>(&neigh_val) - reinterpret_cast<const uint8_t*>(p_view.head_ptr);
                const int64_t neigh_g_idx = byte_diff / stride;
                
                if (neigh_g_idx >= 0 && neigh_g_idx < total_capacity && global_to_local[neigh_g_idx] != -1) {
                    const int32_t local_neigh = global_to_local[neigh_g_idx];
                    if (local_neigh > i && _evaluate(val_a, neigh_val)) {
                        _unite(dsu_map, i, local_neigh);
                    }
                }
            }
        }

        // 4. Calculate cluster sizes & 5. Assign Partitions
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
    inline int32_t _find_root(int32_t* dsu_map, int32_t i) const {
        while (i != dsu_map[i]) {
            dsu_map[i] = dsu_map[dsu_map[i]]; // Path compression
            i = dsu_map[i];
        }
        return i;
    }

    inline void _unite(int32_t* dsu_map, int32_t i, int32_t j) const {
        int32_t root_i = _find_root(dsu_map, i);
        int32_t root_j = _find_root(dsu_map, j);
        if (root_i != root_j) dsu_map[root_j] = root_i;
    }

    inline bool _evaluate(const T& a, const T& b) const {
        if (mode == ClusterCompareMode::ABSOLUTE_VALUE) {
            if constexpr (std::is_floating_point_v<T>) return std::abs(a - b) <= tolerance;
            else if constexpr (requires { a.distance_squared_to(b); }) return a.distance_squared_to(b) <= (tolerance * tolerance);
            else return a == b;
        } else if (mode == ClusterCompareMode::BITMASK_MATCH) {
            if constexpr (std::is_integral_v<T>) return (a & bitmask) == (b & bitmask);
        }
        return false;
    }

    inline void _cull_element(MemoryBufferSelectionPOD& r_selection, int64_t idx) const {
        if (r_selection.mode == SelectionMode::DENSE) {
            r_selection.data.bitset[idx >> 6] &= ~(1ULL << (idx & 63));
            r_selection.element_count--;
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            for (int64_t i = 0; i < r_selection.element_count; ++i) {
                if (r_selection.data.indices[i] == idx) {
                    r_selection.data.indices[i] = -1; // Marker for repack
                    break;
                }
            }
        }
    }

    inline void _repack_sparse(MemoryBufferSelectionPOD& r_selection) const {
        int64_t write_ptr = 0;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (r_selection.data.indices[i] != -1) {
                r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_DSU_CLUSTER_METADATA_LOGIC_H