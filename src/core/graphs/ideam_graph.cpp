#include "ideam_graph.h"

namespace ideam::core {

void IdeamGraph::reserve(size_t p_node_count, size_t p_edge_count) {
    nodes.reserve(p_node_count);
    edges.reserve(p_edge_count);
}

NodeID IdeamGraph::add_node(uint32_t p_type_id) {
    NodeID new_id = static_cast<NodeID>(nodes.size());
    nodes.emplace_back();
    GraphNodeData& node = nodes.back();
    node.id = new_id;
    node.type_id = p_type_id;
    
    dirty_flags |= STRUCTURE;
    return new_id;
}

void IdeamGraph::remove_node(NodeID p_id) {
    if (p_id >= nodes.size() || nodes[p_id].id == INVALID_ID) return;

    for (uint32_t i = 0; i < edges.size(); ++i) {
        if (edges[i].id != INVALID_ID && (edges[i].from_node == p_id || edges[i].to_node == p_id)) {
            disconnect_nodes(i);
        }
    }

    nodes[p_id].id = INVALID_ID;
    dirty_flags |= STRUCTURE;
}

void IdeamGraph::set_node_priority(NodeID p_id, int32_t p_priority) {
    if (p_id < nodes.size() && nodes[p_id].id != INVALID_ID) {
        nodes[p_id].execution_priority = p_priority;
        dirty_flags |= PRIORITY;
    }
}

EdgeID IdeamGraph::connect_nodes(NodeID p_from, uint32_t p_from_port, NodeID p_to, uint32_t p_to_port) {
    if (p_from >= nodes.size() || p_to >= nodes.size()) return INVALID_ID;
    if (nodes[p_from].id == INVALID_ID || nodes[p_to].id == INVALID_ID) return INVALID_ID;

    EdgeID new_id = static_cast<EdgeID>(edges.size());
    edges.push_back({new_id, p_from, p_to, p_from_port, p_to_port});

    dirty_flags |= CONNECTIONS;
    return new_id;
}

void IdeamGraph::disconnect_nodes(EdgeID p_edge_id) {
    if (p_edge_id >= edges.size() || edges[p_edge_id].id == INVALID_ID) return;

    edges[p_edge_id].id = INVALID_ID;
    edges[p_edge_id].from_node = INVALID_ID;
    edges[p_edge_id].to_node = INVALID_ID;
    dirty_flags |= CONNECTIONS;
}

const std::vector<std::vector<NodeID>>& IdeamGraph::get_execution_waves() {
    if (dirty_flags != CLEAN) {
        _rebuild_topology();
    }
    return execution_waves;
}

void IdeamGraph::_rebuild_topology() {
    if (dirty_flags & (STRUCTURE | CONNECTIONS)) {
        _bake_adjacency();
    }
    
    // Always re-sort if structure, connections, or priorities changed
    _sort_kahn_waves();
    
    on_topology_changed();
    dirty_flags = CLEAN;
}

/**
 * _bake_adjacency
 * Three-pass CSR builder. Eliminates all intermediate vector allocations.
 */
void IdeamGraph::_bake_adjacency() {
    // Count active edges to size registries exactly
    uint32_t active_edges = 0;
    for (const auto& edge : edges) {
        if (edge.id != INVALID_ID) active_edges++;
    }

    input_registry.assign(active_edges, INVALID_ID);
    output_registry.assign(active_edges, INVALID_ID);
    
    // Pass 1: Zero out counts
    for (auto& node : nodes) {
        node.input_edge_count = 0;
        node.output_edge_count = 0;
    }

    // Pass 2: Count per-node requirements
    for (const auto& edge : edges) {
        if (edge.id == INVALID_ID) continue;
        nodes[edge.from_node].output_edge_count++;
        nodes[edge.to_node].input_edge_count++;
    }

    // Pass 3: Calculate offsets (Prefix Sum) and reset counts for pointer tracking
    uint32_t current_in_offset = 0;
    uint32_t current_out_offset = 0;
    for (auto& node : nodes) {
        node.input_edge_offset = current_in_offset;
        node.output_edge_offset = current_out_offset;
        
        current_in_offset += node.input_edge_count;
        current_out_offset += node.output_edge_count;
        
        // Temporarily reset to use as iterators for filling
        node.input_edge_count = 0;
        node.output_edge_count = 0;
    }

    // Pass 4: Final Fill
    for (const auto& edge : edges) {
        if (edge.id == INVALID_ID) continue;
        
        GraphNodeData& from = nodes[edge.from_node];
        output_registry[from.output_edge_offset + from.output_edge_count++] = edge.id;
        
        GraphNodeData& to = nodes[edge.to_node];
        input_registry[to.input_edge_offset + to.input_edge_count++] = edge.id;
    }
}

void IdeamGraph::_sort_kahn_waves() {
    execution_waves.clear();
    if (nodes.empty()) return;

    std::vector<int32_t> in_degree(nodes.size(), 0);
    std::vector<NodeID> current_wave;

    for (const auto& edge : edges) {
        if (edge.id != INVALID_ID) {
            in_degree[edge.to_node]++;
        }
    }

    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id != INVALID_ID && in_degree[i] == 0) {
            current_wave.push_back(i);
        }
    }

    while (!current_wave.empty()) {
        // Deterministic sort within wave based on user priority
        std::sort(current_wave.begin(), current_wave.end(), [this](NodeID a, NodeID b) {
            return nodes[a].execution_priority > nodes[b].execution_priority;
        });

        execution_waves.push_back(current_wave);
        std::vector<NodeID> next_wave;

        for (NodeID u : current_wave) {
            uint32_t count = 0;
            const EdgeID* out_edges = get_output_edges(u, count);
            for (uint32_t i = 0; i < count; ++i) {
                NodeID v = edges[out_edges[i]].to_node;
                if (--in_degree[v] == 0) {
                    next_wave.push_back(v);
                }
            }
        }
        current_wave = std::move(next_wave);
    }
}

void IdeamGraph::defragment() {
    if (nodes.empty() && edges.empty()) return;

    std::vector<NodeID> node_lut(nodes.size(), INVALID_ID);
    std::vector<EdgeID> edge_lut(edges.size(), INVALID_ID);

    std::vector<GraphNodeData> new_nodes;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id != INVALID_ID) {
            node_lut[i] = static_cast<NodeID>(new_nodes.size());
            new_nodes.push_back(nodes[i]);
            new_nodes.back().id = node_lut[i];
        }
    }

    std::vector<GraphEdgeData> new_edges;
    for (uint32_t i = 0; i < edges.size(); ++i) {
        if (edges[i].id != INVALID_ID) {
            edge_lut[i] = static_cast<EdgeID>(new_edges.size());
            new_edges.push_back(edges[i]);
            new_edges.back().id = edge_lut[i];
        }
    }

    nodes = std::move(new_nodes);
    edges = std::move(new_edges);
    _remap_ids(node_lut, edge_lut);
    dirty_flags = ALL;
}

void IdeamGraph::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    for (auto& edge : edges) {
        edge.from_node = p_node_lut[edge.from_node];
        edge.to_node = p_node_lut[edge.to_node];
    }
}

const EdgeID* IdeamGraph::get_input_edges(NodeID p_id, uint32_t& r_count) const {
    if (p_id >= nodes.size() || nodes[p_id].id == INVALID_ID) {
        r_count = 0; return nullptr;
    }
    r_count = nodes[p_id].input_edge_count;
    return &input_registry[nodes[p_id].input_edge_offset];
}

const EdgeID* IdeamGraph::get_output_edges(NodeID p_id, uint32_t& r_count) const {
    if (p_id >= nodes.size() || nodes[p_id].id == INVALID_ID) {
        r_count = 0; return nullptr;
    }
    r_count = nodes[p_id].output_edge_count;
    return &output_registry[nodes[p_id].output_edge_offset];
}

GraphNodeData* IdeamGraph::get_node(NodeID p_id) {
    return (p_id < nodes.size() && nodes[p_id].id != INVALID_ID) ? &nodes[p_id] : nullptr;
}

void IdeamGraph::clear() {
    nodes.clear();
    edges.clear();
    input_registry.clear();
    output_registry.clear();
    execution_waves.clear();
    dirty_flags = ALL;
}

} // namespace ideam::core