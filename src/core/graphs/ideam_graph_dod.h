#ifndef IDEAM_CORE_GRAPH_DOD_H
#define IDEAM_CORE_GRAPH_DOD_H

#include "../memory/memory_manager_dod.h"
#include "../memory/memory_common.h"
#include <vector>
#include <cstdint>
#include <algorithm>

namespace ideam::core {

using NodeID = uint32_t;
using EdgeID = uint32_t;
static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

/**
 * GraphDirtyFlags
 * Bitmask to track structural or priority changes.
 */
enum GraphDirtyFlags : uint32_t {
    CLEAN       = 0,
    CONNECTIONS = 1 << 0, 
    PRIORITY    = 1 << 1, 
    STRUCTURE   = 1 << 2, 
    ALL         = 0xFFFFFFFF
};

/**
 * GraphNodeData
 * POD structure for node metadata and CSR adjacency offsets.
 */
struct GraphNodeData {
    NodeID id = INVALID_ID;
    uint32_t type_id = 0;
    int32_t execution_priority = 0;
    
    uint32_t input_edge_offset = 0;
    uint32_t input_edge_count = 0;
    
    uint32_t output_edge_offset = 0;
    uint32_t output_edge_count = 0;
};

/**
 * GraphEdgeData
 * Raw connection data used during the build phase.
 */
struct GraphEdgeData {
    EdgeID id = INVALID_ID;
    NodeID from_node = INVALID_ID;
    NodeID to_node = INVALID_ID;
    uint32_t from_port = 0;
    uint32_t to_port = 0;
};

/**
 * WaveInfo
 * Metadata for locating a specific wave within the flattened wave buffer.
 */
struct WaveInfo {
    uint32_t offset = 0;
    uint32_t count = 0;
};

class IdeamGraphDOD {
protected:
    MemoryManagerDOD* manager = nullptr;
    const uint64_t* global_version_ptr = nullptr;
    uint64_t last_synced_version = 0;

    // --- Build Phase Storage ---
    std::vector<GraphNodeData> build_nodes;
    std::vector<GraphEdgeData> build_edges;

    // --- Execution Phase IDs (Managed by MemoryManagerDOD) ---
    uint32_t input_registry_id = 0xFFFFFFFF;
    uint32_t output_registry_id = 0xFFFFFFFF;
    uint32_t wave_node_id = 0xFFFFFFFF;
    uint32_t wave_meta_id = 0xFFFFFFFF;

    uint32_t dirty_flags = ALL;

    // --- Internal Logic ---
    void _rebuild_topology();
    void _bake_adjacency();
    virtual void _sort_kahn_waves();
    virtual void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut);
    
    /**
     * _ensure_buffer
     * Handles the lack of a native resize in the manager by checking capacity 
     * and recreating the buffer if the footprint changes.
     */
    void _ensure_buffer(uint32_t& r_id, size_t p_size_bytes, uint32_t p_alignment = 64);
    
    bool is_manager_version_dirty() const {
        return global_version_ptr && (*global_version_ptr != last_synced_version);
    }

public:
    explicit IdeamGraphDOD(MemoryManagerDOD* p_manager);
    virtual ~IdeamGraphDOD() = default;

    // Mutation
    NodeID add_node(uint32_t p_type_id);
    void remove_node(NodeID p_id);
    void set_node_priority(NodeID p_id, int32_t p_priority);
    virtual void defragment();

    EdgeID connect_nodes(NodeID p_from, uint32_t p_from_port, NodeID p_to, uint32_t p_to_port);
    void disconnect_nodes(EdgeID p_edge_id);

    void reserve(size_t p_node_count, size_t p_edge_count);
    void clear();

    // DOD Access
    bool bake_topology_grant(MemoryGrantPOD& r_grant);
    
    size_t get_node_count() const { return build_nodes.size(); }
    uint32_t get_wave_meta_id() const { return wave_meta_id; }

    virtual void on_topology_changed() {} 
};

} // namespace ideam::core

#endif // IDEAM_CORE_GRAPH_DOD_H