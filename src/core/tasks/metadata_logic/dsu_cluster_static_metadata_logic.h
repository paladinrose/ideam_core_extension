#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <cmath>
#include <unordered_map>

namespace ideam::core {

enum class ClusterStaticCompareMode : uint8_t {
    ABSOLUTE_VALUE,
    ANGULAR,
    BITMASK_MATCH
};

/**
 * DSUClusterStaticMetadataLogic<T, PointCount>
 * Clusters adjacent elements into partitions using a Disjoint Set Union algorithm.
 * Fully delegates spatial topology to the provided StaticStencilView.
 * TRANSIENT MEMORY: Requires `capacity * 20` bytes for O(N) mapping arrays.
 */
template <typename T, size_t PointCount>
struct DSUClusterStaticMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, PointCount>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY_NUMERIC | DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Topological/Graph evaluation assumes dimensionless topology
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size = PointCount;
    
    static constexpr size_t transient_workspace_bytes     = 0; // User must set via Graph to `capacity * 20`

    // --- Configuration ---
    ClusterStaticCompareMode mode = ClusterStaticCompareMode::ABSOLUTE_VALUE;
    
    float tolerance = 0.1f;
    float cos_threshold = 1.0f;
    uint32_t bitmask = 0xFFFFFFFF;

    int64_t min_cluster_size = 1;
    int32_t partition_id_offset = 0;
    uint32_t target_buffer_id = 0;

    static godot::Array get_ui_properties() {
        godot::Array props;

        // 1. Mode
        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Absolute Value,Angular,Bitmask Match";
        props.push_back(mode_prop);

        // 2. Tolerance
        godot::Dictionary tol_prop;
        tol_prop["name"] = "tolerance";
        tol_prop["type"] = godot::Variant::FLOAT;
        tol_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tol_prop);

        // 3. Cos Threshold
        godot::Dictionary cos_prop;
        cos_prop["name"] = "cos_threshold";
        cos_prop["type"] = godot::Variant::FLOAT;
        cos_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cos_prop);

        // 4. Bitmask
        godot::Dictionary bit_prop;
        bit_prop["name"] = "bitmask";
        bit_prop["type"] = godot::Variant::INT;
        bit_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(bit_prop);

        // 5. Min Cluster Size
        godot::Dictionary min_prop;
        min_prop["name"] = "min_cluster_size";
        min_prop["type"] = godot::Variant::INT;
        min_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(min_prop);

        // 6. Partition ID Offset
        godot::Dictionary part_offset_prop;
        part_offset_prop["name"] = "partition_id_offset";
        part_offset_prop["type"] = godot::Variant::INT;
        part_offset_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(part_offset_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("mode")) {
            mode = static_cast<ClusterStaticCompareMode>(static_cast<uint8_t>(static_cast<int64_t>(p_props["mode"])));
        }
        if (p_props.has("tolerance")) {
            tolerance = static_cast<float>(p_props["tolerance"]);
        }
        if (p_props.has("cos_threshold")) {
            cos_threshold = static_cast<float>(p_props["cos_threshold"]);
        }
        if (p_props.has("bitmask")) {
            bitmask = static_cast<uint32_t>(static_cast<int64_t>(p_props["bitmask"]));
        }
        if (p_props.has("min_cluster_size")) {
            min_cluster_size = static_cast<int64_t>(p_props["min_cluster_size"]);
        }
        if (p_props.has("partition_id_offset")) {
            partition_id_offset = static_cast<int32_t>(static_cast<int64_t>(p_props["partition_id_offset"]));
        }
    }

    template <typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection,
                            const TaskContextPOD& p_context,
                            const T_View& p_view
                            //const T_Strategy& p_strategy,
                            ) const {
        if (!r_selection.partition_ids || r_selection.element_count == 0 || !p_context.local_workspace) return;

        const int64_t total_capacity = r_selection.capacity;
        
        // 1. Transient Arena Allocations (O(1) stack-like allocation)
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

        int32_t* dsu_map = reinterpret_cast<int32_t*>(local_to_global + total_capacity);
        int32_t* root_sizes = dsu_map + total_capacity;

        // 2. Initialize DSU Map
        for (int32_t i = 0; i < n; ++i) {
            dsu_map[i] = i;
            root_sizes[i] = 0;
        }

        const GrantPartPOD* part = p_context.get_grant_part(target_buffer_id);
        const intptr_t stride = part->element_stride;
        
        // SINGLE SOURCE OF TRUTH: Hardware base pointer from the Grant
        const uint8_t* raw_base = static_cast<const uint8_t*>(part->raw_base_ptr);

        // 3. Unify Neighbors
        for (int32_t i = 0; i < n; ++i) {
            // CLEAN ABSTRACTION: 
            // 'i' maps perfectly to the View's logical selection coordinate.
            // Calling operator[] correctly positions the View's internal mutable cursor.
            const T& val_a = p_view[i];

            // C++17 compile-time gate ensures SingleElementView (PointCount = 0) is pruned 
            // and never attempts to call neighbor().
            if constexpr (PointCount > 0) {
                for (size_t p = 0; p < PointCount; ++p) {
                    const T& neigh_val = p_view.neighbor(p);
                    
                    // SAFE HARDWARE MATH: Calculate global index offset against the GrantPart base, 
                    // completely independent of the View's internal naming structure.
                    const intptr_t byte_diff = reinterpret_cast<const uint8_t*>(&neigh_val) - raw_base;
                    const int64_t neigh_g_idx = byte_diff / stride;
                    
                    if (neigh_g_idx >= 0 && neigh_g_idx < total_capacity && global_to_local[neigh_g_idx] != -1) {
                        const int32_t local_neigh = global_to_local[neigh_g_idx];
                        if (local_neigh > i && _evaluate(val_a, neigh_val)) {
                            _unite(dsu_map, i, local_neigh);
                        }
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
        if (mode == ClusterStaticCompareMode::ABSOLUTE_VALUE) {
            if constexpr (std::is_floating_point_v<T>) return std::abs(a - b) <= tolerance;
            else if constexpr (requires { a.distance_squared_to(b); }) return a.distance_squared_to(b) <= (tolerance * tolerance);
            else return a == b;
        } else if (mode == ClusterStaticCompareMode::BITMASK_MATCH) {
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