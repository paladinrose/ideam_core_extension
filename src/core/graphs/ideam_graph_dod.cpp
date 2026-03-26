#include "ideam_graph_dod.h"
#include <cstring>

namespace ideam::core {

IdeamGraphDOD::IdeamGraphDOD(MemoryManagerDOD* p_manager) : manager(p_manager) {
    static_assert(MemoryUtilities::is_dod_safe<GraphNodeData>(), "GraphNodeData must be POD.");
    static_assert(MemoryUtilities::is_dod_safe<GraphEdgeData>(), "GraphEdgeData must be POD.");
    static_assert(MemoryUtilities::is_dod_safe<WaveInfo>(), "WaveInfo must be POD.");
    
    if (manager) {
        global_version_ptr = manager->get_global_version_ptr();
    }
}

NodeID IdeamGraphDOD::add_node(uint32_t p_type_id) {
    NodeID new_id = static_cast<NodeID>(build_nodes.size());
    build_nodes.emplace_back();
    GraphNodeData& node = build_nodes.back();
    node.id = new_id;
    node.type_id = p_type_id;
    
    dirty_flags |= STRUCTURE;
    return new_id;
}

void IdeamGraphDOD::remove_node(NodeID p_id) {
    if (p_id >= build_nodes.size() || build_nodes[p_id].id == INVALID_ID) return;

    for (uint32_t i = 0; i < build_edges.size(); ++i) {
        if (build_edges[i].id != INVALID_ID && (build_edges[i].from_node == p_id || build_edges[i].to_node == p_id)) {
            disconnect_nodes(i);
        }
    }

    build_nodes[p_id].id = INVALID_ID;
    dirty_flags |= STRUCTURE;
}

void IdeamGraphDOD::set_node_priority(NodeID p_id, int32_t p_priority) {
    if (p_id < build_nodes.size() && build_nodes[p_id].id != INVALID_ID) {
        build_nodes[p_id].execution_priority = p_priority;
        dirty_flags |= PRIORITY;
    }
}

EdgeID IdeamGraphDOD::connect_nodes(NodeID p_from, uint32_t p_from_port, NodeID p_to, uint32_t p_to_port) {
    if (p_from >= build_nodes.size() || p_to >= build_nodes.size()) return INVALID_ID;
    if (build_nodes[p_from].id == INVALID_ID || build_nodes[p_to].id == INVALID_ID) return INVALID_ID;

    EdgeID new_id = static_cast<EdgeID>(build_edges.size());
    build_edges.push_back({new_id, p_from, p_to, p_from_port, p_to_port});

    dirty_flags |= CONNECTIONS;
    return new_id;
}

void IdeamGraphDOD::disconnect_nodes(EdgeID p_edge_id) {
    if (p_edge_id >= build_edges.size() || build_edges[p_edge_id].id == INVALID_ID) return;

    build_edges[p_edge_id].id = INVALID_ID;
    build_edges[p_edge_id].from_node = INVALID_ID;
    build_edges[p_edge_id].to_node = INVALID_ID;
    dirty_flags |= CONNECTIONS;
}

bool IdeamGraphDOD::bake_topology_grant(MemoryGrantPOD& r_grant) {
    if (dirty_flags != CLEAN) {
        _rebuild_topology();
    }

    std::vector<GrantPartPOD> reqs;
    
    // Using designated initializers to avoid constructor ambiguity
    reqs.push_back(GrantPartPOD{ .buffer_id = input_registry_id, .access_mode = BufferAccessMode::READ });
    reqs.push_back(GrantPartPOD{ .buffer_id = output_registry_id, .access_mode = BufferAccessMode::READ });
    reqs.push_back(GrantPartPOD{ .buffer_id = wave_node_id, .access_mode = BufferAccessMode::READ });
    reqs.push_back(GrantPartPOD{ .buffer_id = wave_meta_id, .access_mode = BufferAccessMode::READ });

    return manager->bake_grant(r_grant, reqs);
}

void IdeamGraphDOD::_rebuild_topology() {
    if (dirty_flags & (STRUCTURE | CONNECTIONS)) {
        _bake_adjacency();
    }
    _sort_kahn_waves();
    
    on_topology_changed();
    dirty_flags = CLEAN;
}

void IdeamGraphDOD::_bake_adjacency() {
    uint32_t active_edges = 0;
    for (const auto& edge : build_edges) {
        if (edge.id != INVALID_ID) active_edges++;
    }

    _ensure_buffer(input_registry_id, active_edges * sizeof(EdgeID));
    _ensure_buffer(output_registry_id, active_edges * sizeof(EdgeID));

    MemoryGrantPOD in_grant, out_grant;
    manager->bake_grant(in_grant, { 
        GrantPartPOD{ .buffer_id = input_registry_id, .access_mode = BufferAccessMode::WRITE } 
    });
    
    manager->bake_grant(out_grant, { 
        GrantPartPOD{ .buffer_id = output_registry_id, .access_mode = BufferAccessMode::WRITE } 
    });

    EdgeID* in_ptr = reinterpret_cast<EdgeID*>(in_grant.parts[0].raw_base_ptr);
    EdgeID* out_ptr = reinterpret_cast<EdgeID*>(out_grant.parts[0].raw_base_ptr);

    for (auto& node : build_nodes) {
        node.input_edge_count = 0;
        node.output_edge_count = 0;
    }

    for (const auto& edge : build_edges) {
        if (edge.id == INVALID_ID) continue;
        build_nodes[edge.from_node].output_edge_count++;
        build_nodes[edge.to_node].input_edge_count++;
    }

    uint32_t current_in = 0;
    uint32_t current_out = 0;
    for (auto& node : build_nodes) {
        node.input_edge_offset = current_in;
        node.output_edge_offset = current_out;
        current_in += node.input_edge_count;
        current_out += node.output_edge_count;
        node.input_edge_count = 0;
    }

    for (const auto& edge : build_edges) {
        if (edge.id == INVALID_ID) continue;
        GraphNodeData& f = build_nodes[edge.from_node];
        out_ptr[f.output_edge_offset + f.output_edge_count++] = edge.id;
        
        GraphNodeData& t = build_nodes[edge.to_node];
        in_ptr[t.input_edge_offset + t.input_edge_count++] = edge.id;
    }

    manager->release_grant(in_grant);
    manager->release_grant(out_grant);
}

void IdeamGraphDOD::_sort_kahn_waves() {
    if (build_nodes.empty()) return;

    std::vector<int32_t> in_degree(build_nodes.size(), 0);
    std::vector<std::vector<NodeID>> wave_list;
    std::vector<NodeID> current_wave;

    for (const auto& edge : build_edges) {
        if (edge.id != INVALID_ID) in_degree[edge.to_node]++;
    }

    for (uint32_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes[i].id != INVALID_ID && in_degree[i] == 0) current_wave.push_back(i);
    }

    size_t total_node_count = 0;
    while (!current_wave.empty()) {
        std::sort(current_wave.begin(), current_wave.end(), [this](NodeID a, NodeID b) {
            return build_nodes[a].execution_priority > build_nodes[b].execution_priority;
        });

        total_node_count += current_wave.size();
        wave_list.push_back(current_wave);
        std::vector<NodeID> next_wave;

        for (NodeID u : current_wave) {
            const GraphNodeData& node = build_nodes[u];

            MemoryGrantPOD out_grant;
            manager->bake_grant(out_grant, {
                GrantPartPOD{ .buffer_id = output_registry_id, .access_mode = BufferAccessMode::READ } 
            });

            EdgeID* out_ptr = reinterpret_cast<EdgeID*>(out_grant.parts[0].raw_base_ptr);

            for (uint32_t i = 0; i < node.output_edge_count; ++i) {
                EdgeID eid = out_ptr[node.output_edge_offset + i];
                NodeID v = build_edges[eid].to_node;
                if (--in_degree[v] == 0) next_wave.push_back(v);
            }
            manager->release_grant(out_grant);
        }
        current_wave = std::move(next_wave);
    }

    _ensure_buffer(wave_node_id, total_node_count * sizeof(NodeID));
    _ensure_buffer(wave_meta_id, wave_list.size() * sizeof(WaveInfo));

    MemoryGrantPOD n_grant, m_grant;
    manager->bake_grant(n_grant, { 
        GrantPartPOD{ .buffer_id = wave_node_id, .access_mode = BufferAccessMode::WRITE } 
    });
    
    manager->bake_grant(m_grant, { 
        GrantPartPOD{ .buffer_id = wave_meta_id, .access_mode = BufferAccessMode::WRITE } 
    });
    
    NodeID* n_ptr = reinterpret_cast<NodeID*>(n_grant.parts[0].raw_base_ptr);
    WaveInfo* m_ptr = reinterpret_cast<WaveInfo*>(m_grant.parts[0].raw_base_ptr);

    uint32_t write_ptr = 0;
    for (uint32_t w = 0; w < wave_list.size(); ++w) {
        m_ptr[w] = { write_ptr, static_cast<uint32_t>(wave_list[w].size()) };
        for (NodeID id : wave_list[w]) {
            n_ptr[write_ptr++] = id;
        }
    }
    manager->release_grant(n_grant);
    manager->release_grant(m_grant);
}

void IdeamGraphDOD::_ensure_buffer(uint32_t& r_id, size_t p_size_bytes, uint32_t p_alignment) {
    if (r_id != 0xFFFFFFFF) {
        MemoryBufferPOD* buf = manager->get_buffer(r_id);
        if (buf && buf->capacity_bytes >= p_size_bytes) return;
        // In current MemoryManagerDOD, we cannot destroy or resize individual buffers 
        // without affecting the master block offsets of subsequent buffers.
        // If p_size_bytes exceeds capacity, a rebase/re-creation strategy is needed.
    }
    r_id = manager->create_buffer(BufferLayoutType::FLAT, p_size_bytes, p_alignment);
}

void IdeamGraphDOD::reserve(size_t p_node_count, size_t p_edge_count) {
    build_nodes.reserve(p_node_count);
    build_edges.reserve(p_edge_count);
}

void IdeamGraphDOD::defragment() {
    if (build_nodes.empty() && build_edges.empty()) {
        clear();
        return;
    }

    // 1. Create Lookup Tables (LUT) to map Old Indices -> New Indices
    std::vector<NodeID> node_lut(build_nodes.size(), INVALID_ID);
    std::vector<EdgeID> edge_lut(build_edges.size(), INVALID_ID);

    // 2. Pack Nodes
    std::vector<GraphNodeData> packed_nodes;
    packed_nodes.reserve(build_nodes.size());
    
    for (uint32_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes[i].id != INVALID_ID) {
            node_lut[i] = static_cast<NodeID>(packed_nodes.size());
            packed_nodes.push_back(build_nodes[i]);
            // Update the internal ID to match the new vector index
            packed_nodes.back().id = node_lut[i];
        }
    }

    // 3. Pack Edges
    std::vector<GraphEdgeData> packed_edges;
    packed_edges.reserve(build_edges.size());

    for (uint32_t i = 0; i < build_edges.size(); ++i) {
        if (build_edges[i].id != INVALID_ID) {
            // Verify that the nodes this edge connects still exist
            NodeID new_from = node_lut[build_edges[i].from_node];
            NodeID new_to = node_lut[build_edges[i].to_node];

            if (new_from != INVALID_ID && new_to != INVALID_ID) {
                edge_lut[i] = static_cast<EdgeID>(packed_edges.size());
                packed_edges.push_back(build_edges[i]);
                
                // Update Edge metadata with new IDs
                GraphEdgeData& edge = packed_edges.back();
                edge.id = edge_lut[i];
                edge.from_node = new_from;
                edge.to_node = new_to;
            }
        }
    }

    // 4. Swap existing storage with packed storage
    build_nodes = std::move(packed_nodes);
    build_edges = std::move(packed_edges);

    // 5. Invoke ID Remapping for derived classes
    // This allows MemoryGraphDOD to shift its metadata/grants using the LUTs
    _remap_ids(node_lut, edge_lut);

    // 6. Force a full topology rebuild
    dirty_flags = ALL;
    _rebuild_topology();
}

void IdeamGraphDOD::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    // Base implementation is a no-op. 
    // Derived classes like MemoryGraphDOD override this to shuffle parallel arrays.
}

void IdeamGraphDOD::clear() {
    build_nodes.clear();
    build_edges.clear();
    dirty_flags = ALL;
}

} // namespace ideam::core