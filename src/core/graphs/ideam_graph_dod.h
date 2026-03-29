#ifndef IDEAM_CORE_GRAPH_DOD_H
#define IDEAM_CORE_GRAPH_DOD_H

#include "../memory/memory_manager_dod.h"
#include "../memory/memory_common.h"
#include "../memory/views/paged_view.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <array>
#include <bit>

namespace ideam::core {

using NodeID = uint32_t;
using EdgeID = uint32_t;
static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

/**
 * @enum GraphDirtyFlags
 * @brief Bitmask to track structural or priority changes within the graph topology.
 * Allows the execution phase to intelligently bypass unnecessary rebuilds.
 */
enum GraphDirtyFlags : uint32_t {
    CLEAN       = 0,
    CONNECTIONS = 1 << 0, 
    PRIORITY    = 1 << 1, 
    STRUCTURE   = 1 << 2, 
    ALL         = 0xFFFFFFFF
};

/**
 * @struct BuildNodesSoA
 * @brief Structure of Arrays (SoA) layout for the mutable graph build phase.
 * Guarantees perfect cache-line utilization during topological sorting and adjacency baking
 * by preventing cold data (like type_ids) from polluting the L1 cache during priority evaluations.
 */
struct BuildNodesSoA {
    std::vector<NodeID> id;
    std::vector<uint32_t> type_id;
    std::vector<int32_t> execution_priority;
    
    std::vector<uint32_t> input_edge_offset;
    std::vector<uint32_t> input_edge_count;
    
    std::vector<uint32_t> output_edge_offset;
    std::vector<uint32_t> output_edge_count;

    [[nodiscard]] size_t size() const noexcept { return id.size(); }
    [[nodiscard]] bool empty() const noexcept { return id.empty(); }

    void clear() {
        id.clear();
        type_id.clear();
        execution_priority.clear();
        input_edge_offset.clear();
        input_edge_count.clear();
        output_edge_offset.clear();
        output_edge_count.clear();
    }

    void reserve(size_t p_capacity) {
        id.reserve(p_capacity);
        type_id.reserve(p_capacity);
        execution_priority.reserve(p_capacity);
        input_edge_offset.reserve(p_capacity);
        input_edge_count.reserve(p_capacity);
        output_edge_offset.reserve(p_capacity);
        output_edge_count.reserve(p_capacity);
    }

    void push_back(NodeID p_id, uint32_t p_type_id, int32_t p_priority = 0) {
        id.push_back(p_id);
        type_id.push_back(p_type_id);
        execution_priority.push_back(p_priority);
        input_edge_offset.push_back(0);
        input_edge_count.push_back(0);
        output_edge_offset.push_back(0);
        output_edge_count.push_back(0);
    }
};

/**
 * @struct GraphEdgeData
 * @brief Raw connection data used exclusively during the build phase. 
 * Retained as an Array-of-Structures (AoS) as it is accessed strictly sequentially during the adjacency bake.
 */
struct GraphEdgeData {
    EdgeID id = INVALID_ID;
    NodeID from_node = INVALID_ID;
    NodeID to_node = INVALID_ID;
    uint32_t from_port = 0;
    uint32_t to_port = 0;
};

/**
 * @struct WaveInfo
 * @brief Execution phase metadata for locating a specific parallel wave within the flattened wave buffer.
 */
struct WaveInfo {
    uint32_t offset = 0;
    uint32_t count = 0;
};

/**
 * @class IdeamGraphDOD
 * @brief High-performance, memory-aligned directed graph architecture.
 * Transforms a mutable build-phase topology into a cache-perfect, immutable execution sequence.
 */
class IdeamGraphDOD {
protected:
    MemoryManagerDOD* manager = nullptr;
    const uint64_t* global_version_ptr = nullptr;
    uint64_t last_synced_version = 0;

    // --- Build Phase Storage (SoA Optimized) ---
    BuildNodesSoA build_nodes;
    std::vector<GraphEdgeData> build_edges;

    // --- Execution Phase IDs (Managed by MemoryManagerDOD) ---
    uint32_t input_registry_id = 0xFFFFFFFF;
    uint32_t output_registry_id = 0xFFFFFFFF;
    uint32_t wave_node_id = 0xFFFFFFFF;
    uint32_t wave_meta_id = 0xFFFFFFFF;

    uint32_t dirty_flags = ALL;

    // --- Internal Logic ---
    virtual void _rebuild_topology();
    void _bake_adjacency();
    virtual void _sort_kahn_waves();
    virtual void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut);
    
    /**
     * @brief _ensure_buffer
     * Upgraded to utilize PAGED memory buffers. Allows infinite virtual growth without forcing 
     * the underlying MemoryManagerDOD to rebase or fragment the master block.
     */
    void _ensure_buffer(uint32_t& r_id, size_t p_size_bytes, uint32_t p_alignment = 64);
    
    /**
     * @brief _get_paged_view
     * Hardware-aware factory for deriving a branchless PagedView directly from an active grant.
     * Inherited and utilized by child graph implementations.
     */
    template<typename T, typename Strategy = FlatStrategy>
    PagedView<T, Strategy> _get_paged_view(const MemoryGrantPOD& p_grant, uint32_t p_part_index) const {
        PagedView<T, Strategy> view;
        view.grant = &p_grant;
        view.grant_part_index = p_part_index;
        view.page_table = reinterpret_cast<uint8_t**>(p_grant.parts[p_part_index].raw_base_ptr);

        MemoryBufferPOD* buf = manager->get_buffer(p_grant.parts[p_part_index].buffer_id);
        uint32_t page_size = buf->extra.paged.page_size_bytes;

        view.page_shift = static_cast<uint32_t>(std::countr_zero(page_size));
        view.page_mask = page_size - 1;
        view.baked_buffer_version = p_grant.parts[p_part_index].buffer_version_at_issue;
        view.baked_manager_version = p_grant.global_manager_version_ptr ? *p_grant.global_manager_version_ptr : 0;

        return view;
    }
    
    bool is_manager_version_dirty() const {
        return global_version_ptr && (*global_version_ptr != last_synced_version);
    }

public:
    explicit IdeamGraphDOD(MemoryManagerDOD* p_manager);
    virtual ~IdeamGraphDOD() = default;

    // --- Mutation Methods ---
    NodeID add_node(uint32_t p_type_id);
    void remove_node(NodeID p_id);
    void set_node_priority(NodeID p_id, int32_t p_priority);
    
    /**
     * @brief defragment
     * Purges INVALID_ID holes from the build phase vectors, compacting memory footprint.
     * Invokes virtual _remap_ids to allow derived classes to synchronize parallel execution data.
     */
    virtual void defragment();

    EdgeID connect_nodes(NodeID p_from, uint32_t p_from_port, NodeID p_to, uint32_t p_to_port);
    void disconnect_nodes(EdgeID p_edge_id);

    void reserve(size_t p_node_count, size_t p_edge_count);
    void clear();

    // --- DOD Access ---
    /**
     * @brief bake_topology_grant
     * Flushes dirty flags and locks a read-access MemoryGrant across all execution registries.
     * Provides thread-safe, version-secured access to the flattened graph logic.
     */
    bool bake_topology_grant(MemoryGrantPOD& r_grant);
    
    size_t get_node_count() const { return build_nodes.size(); }
    uint32_t get_wave_meta_id() const { return wave_meta_id; }

    /**
     * @brief on_topology_changed
     * Virtual hook for derived architectures to intercept graph rebuild events.
     */
    virtual void on_topology_changed() {} 
};

} // namespace ideam::core

#endif // IDEAM_CORE_GRAPH_DOD_H