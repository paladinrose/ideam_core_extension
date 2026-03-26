#ifndef IDEAM_CORE_GRAPH_H
#define IDEAM_CORE_GRAPH_H

#include <vector>
#include <cstdint>
#include <algorithm>

namespace ideam::core {

using NodeID = uint32_t;
using EdgeID = uint32_t;
static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

/**
 * GraphDirtyFlags
 * Bitmask to track specific structural changes.
 */
enum GraphDirtyFlags : uint32_t {
    CLEAN       = 0,
    CONNECTIONS = 1 << 0, // Edges added/removed: Re-bake registries
    PRIORITY    = 1 << 1, // Node weights changed: Re-run Kahn Sort
    STRUCTURE   = 1 << 2, // Nodes added/removed: Affects everything
    ALL         = 0xFFFFFFFF
};

/**
 * GraphNodeData
 * Optimized for cache-friendly execution. Adjacency is stored as offsets
 * into the master Edge Registry rather than individual vectors.
 */
struct GraphNodeData {
    NodeID id = INVALID_ID;
    uint32_t type_id = 0;
    int32_t execution_priority = 0;
    
    // Baked Adjacency Offsets (CSR Format)
    uint32_t input_edge_offset = 0;
    uint32_t input_edge_count = 0;
    
    uint32_t output_edge_offset = 0;
    uint32_t output_edge_count = 0;
};

/**
 * GraphEdgeData
 * The raw connection data used during the "Build" phase.
 */
struct GraphEdgeData {
    EdgeID id = INVALID_ID;
    NodeID from_node = INVALID_ID;
    NodeID to_node = INVALID_ID;
    uint32_t from_port = 0;
    uint32_t to_port = 0;
};

class IdeamGraph {
protected:
    // Structural Storage
    std::vector<GraphNodeData> nodes;
    std::vector<GraphEdgeData> edges;
    
    // The Edge Registry (The "Baked" Adjacency)
    std::vector<EdgeID> input_registry;
    std::vector<EdgeID> output_registry;

    // Execution State
    std::vector<std::vector<NodeID>> execution_waves;
    uint32_t dirty_flags = ALL;

    // --- Internal Logic ---
    void _rebuild_topology();
    void _sort_kahn_waves();
    void _bake_adjacency();
    void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut);

public:
    IdeamGraph() = default;
    virtual ~IdeamGraph() = default;

    // --- Mutation API ---
    NodeID add_node(uint32_t p_type_id);
    void remove_node(NodeID p_id);
    void set_node_priority(NodeID p_id, int32_t p_priority);
    void defragment();

    EdgeID connect_nodes(NodeID p_from, uint32_t p_from_port, NodeID p_to, uint32_t p_to_port);
    void disconnect_nodes(EdgeID p_edge_id);

    // Bulk Management
    void reserve(size_t p_node_count, size_t p_edge_count);

    // --- Accessors ---
    const std::vector<std::vector<NodeID>>& get_execution_waves();
    
    const EdgeID* get_input_edges(NodeID p_id, uint32_t& r_count) const;
    const EdgeID* get_output_edges(NodeID p_id, uint32_t& r_count) const;

    GraphNodeData* get_node(NodeID p_id);
    size_t get_node_count() const { return nodes.size(); }
    void clear();

    virtual void on_topology_changed() {} 
};

} // namespace ideam::core

#endif // IDEAM_CORE_GRAPH_H