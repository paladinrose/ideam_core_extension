#include "ideam_graph_dod.h"
#include <cstring>
#include <ranges>

namespace ideam::core {

IdeamGraphDOD::IdeamGraphDOD(MemoryManagerDOD* p_manager) : manager(p_manager) {
    static_assert(MemoryUtilities::is_dod_safe<GraphEdgeData>(), "GraphEdgeData must be POD.");
    static_assert(MemoryUtilities::is_dod_safe<WaveInfo>(), "WaveInfo must be POD.");
    
    if (manager) {
        global_version_ptr = manager->get_global_version_ptr();
    }
}

NodeID IdeamGraphDOD::add_node(uint32_t p_type_id) {
    NodeID new_id = static_cast<NodeID>(build_nodes.size());
    build_nodes.push_back(new_id, p_type_id);
    
    dirty_flags |= STRUCTURE;
    return new_id;
}

void IdeamGraphDOD::remove_node(NodeID p_id) {
    if (p_id >= build_nodes.size() || build_nodes.id[p_id] == INVALID_ID) return;

    for (uint32_t i = 0; i < build_edges.size(); ++i) {
        if (build_edges[i].id != INVALID_ID && (build_edges[i].from_node == p_id || build_edges[i].to_node == p_id)) {
            disconnect_nodes(i);
        }
    }

    build_nodes.id[p_id] = INVALID_ID;
    dirty_flags |= STRUCTURE;
}

void IdeamGraphDOD::set_node_priority(NodeID p_id, int32_t p_priority) {
    if (p_id < build_nodes.size() && build_nodes.id[p_id] != INVALID_ID) {
        build_nodes.execution_priority[p_id] = p_priority;
        dirty_flags |= PRIORITY;
    }
}

EdgeID IdeamGraphDOD::connect_nodes(NodeID p_from, uint32_t p_from_port, NodeID p_to, uint32_t p_to_port) {
    if (p_from >= build_nodes.size() || p_to >= build_nodes.size()) return INVALID_ID;
    if (build_nodes.id[p_from] == INVALID_ID || build_nodes.id[p_to] == INVALID_ID) return INVALID_ID;

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
    
    std::array<GrantPartPOD, 1> in_req = {{ 
        GrantPartPOD{ .buffer_id = input_registry_id, .access_mode = BufferAccessMode::WRITE } 
    }};
    manager->bake_grant(in_grant, in_req);
    
    std::array<GrantPartPOD, 1> out_req = {{ 
        GrantPartPOD{ .buffer_id = output_registry_id, .access_mode = BufferAccessMode::WRITE } 
    }};
    manager->bake_grant(out_grant, out_req);

    auto in_view = _get_paged_view<EdgeID, FlatStrategy>(in_grant, 0);
    auto out_view = _get_paged_view<EdgeID, FlatStrategy>(out_grant, 0);

    for (size_t i = 0; i < build_nodes.size(); ++i) {
        build_nodes.input_edge_count[i] = 0;
        build_nodes.output_edge_count[i] = 0;
    }

    for (const auto& edge : build_edges) {
        if (edge.id == INVALID_ID) continue;
        build_nodes.output_edge_count[edge.from_node]++;
        build_nodes.input_edge_count[edge.to_node]++;
    }

    uint32_t current_in = 0;
    uint32_t current_out = 0;
    for (size_t i = 0; i < build_nodes.size(); ++i) {
        build_nodes.input_edge_offset[i] = current_in;
        build_nodes.output_edge_offset[i] = current_out;
        current_in += build_nodes.input_edge_count[i];
        current_out += build_nodes.output_edge_count[i];
        build_nodes.input_edge_count[i] = 0; 
    }

    for (const auto& edge : build_edges) {
        if (edge.id == INVALID_ID) continue;
        out_view[build_nodes.output_edge_offset[edge.from_node] + build_nodes.output_edge_count[edge.from_node]++] = edge.id;
        in_view[build_nodes.input_edge_offset[edge.to_node] + build_nodes.input_edge_count[edge.to_node]++] = edge.id;
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
        if (build_nodes.id[i] != INVALID_ID && in_degree[i] == 0) current_wave.push_back(i);
    }

    size_t total_node_count = 0;
    
    // Pre-allocate DOD workspace to prevent heap churn
    std::vector<NodeID> next_wave;
    next_wave.reserve(build_nodes.size());
    std::vector<NodeID> sorted_wave;
    sorted_wave.reserve(build_nodes.size());
    std::vector<uint32_t> bucket_counts;

    while (!current_wave.empty()) {
        // DOD O(N) Linear Counting Sort for Execution Priority
        int32_t max_p = build_nodes.execution_priority[current_wave[0]];
        int32_t min_p = max_p;
        for (NodeID n : current_wave) {
            int32_t p = build_nodes.execution_priority[n];
            if (p > max_p) max_p = p;
            if (p < min_p) min_p = p;
        }
        
        uint32_t priority_range = static_cast<uint32_t>(max_p - min_p + 1);
        if (priority_range < 2048) { // Fast Path: Linear Sweep
            bucket_counts.assign(priority_range, 0);
            for (NodeID n : current_wave) {
                bucket_counts[build_nodes.execution_priority[n] - min_p]++;
            }
            
            std::vector<uint32_t> offsets(priority_range, 0);
            uint32_t current_offset = 0;
            // Iterate backwards to sort descending (highest priority first)
            for (int32_t i = priority_range - 1; i >= 0; --i) {
                offsets[i] = current_offset;
                current_offset += bucket_counts[i];
            }
            
            sorted_wave.resize(current_wave.size());
            for (NodeID n : current_wave) {
                int32_t p_idx = build_nodes.execution_priority[n] - min_p;
                sorted_wave[offsets[p_idx]++] = n;
            }
            std::swap(current_wave, sorted_wave);
        } else {
            // Fallback for extreme outlier priorities (O(N log N))
            std::ranges::sort(current_wave, std::greater{}, [this](NodeID a) {
                return build_nodes.execution_priority[a];
            });
        }

        total_node_count += current_wave.size();
        wave_list.push_back(current_wave);

        for (NodeID u : current_wave) {
            MemoryGrantPOD out_grant;
            std::array<GrantPartPOD, 1> out_req = {{ 
                GrantPartPOD{ .buffer_id = output_registry_id, .access_mode = BufferAccessMode::READ } 
            }};
            manager->bake_grant(out_grant, out_req);

            auto out_view = _get_paged_view<EdgeID, FlatStrategy>(out_grant, 0);

            for (uint32_t i = 0; i < build_nodes.output_edge_count[u]; ++i) {
                EdgeID eid = out_view[build_nodes.output_edge_offset[u] + i];
                NodeID v = build_edges[eid].to_node;
                if (--in_degree[v] == 0) next_wave.push_back(v);
            }
            manager->release_grant(out_grant);
        }
        
        current_wave.clear();
        std::swap(current_wave, next_wave);
    }

    _ensure_buffer(wave_node_id, total_node_count * sizeof(NodeID));
    _ensure_buffer(wave_meta_id, wave_list.size() * sizeof(WaveInfo));

    MemoryGrantPOD n_grant, m_grant;
    std::array<GrantPartPOD, 1> n_req = {{ 
        GrantPartPOD{ .buffer_id = wave_node_id, .access_mode = BufferAccessMode::WRITE } 
    }};
    manager->bake_grant(n_grant, n_req);
    
    std::array<GrantPartPOD, 1> m_req = {{ 
        GrantPartPOD{ .buffer_id = wave_meta_id, .access_mode = BufferAccessMode::WRITE } 
    }};
    manager->bake_grant(m_grant, m_req);
    
    auto n_view = _get_paged_view<NodeID, FlatStrategy>(n_grant, 0);
    auto m_view = _get_paged_view<WaveInfo, FlatStrategy>(m_grant, 0);

    uint32_t write_ptr = 0;
    for (uint32_t w = 0; w < wave_list.size(); ++w) {
        m_view[w] = { write_ptr, static_cast<uint32_t>(wave_list[w].size()) };
        for (NodeID id : wave_list[w]) {
            n_view[write_ptr++] = id;
        }
    }
    manager->release_grant(n_grant);
    manager->release_grant(m_grant);
}

void IdeamGraphDOD::_ensure_buffer(uint32_t& r_id, size_t p_size_bytes, uint32_t p_alignment) {
    size_t page_size = 4096;
    size_t aligned_size = ((p_size_bytes + page_size - 1) / page_size) * page_size;
    if (aligned_size == 0) aligned_size = page_size;

    if (r_id != 0xFFFFFFFF) {
        MemoryBufferPOD* buf = manager->get_buffer(r_id);
        if (buf && buf->capacity_bytes >= aligned_size) return;
        
        // Zero-copy virtual page expansion (prevents Master Block fragmentation)
        if (manager->expand_paged_buffer(r_id, aligned_size)) return;
    }
    
    r_id = manager->create_buffer(BufferLayoutType::PAGED, aligned_size, p_alignment);
    manager->configure_paged(r_id, static_cast<uint32_t>(page_size));
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

    std::vector<NodeID> node_lut(build_nodes.size(), INVALID_ID);
    std::vector<EdgeID> edge_lut(build_edges.size(), INVALID_ID);

    BuildNodesSoA packed_nodes;
    packed_nodes.reserve(build_nodes.size());
    
    for (size_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes.id[i] != INVALID_ID) {
            node_lut[i] = static_cast<NodeID>(packed_nodes.size());
            packed_nodes.push_back(node_lut[i], build_nodes.type_id[i], build_nodes.execution_priority[i]);
        }
    }

    std::vector<GraphEdgeData> packed_edges;
    packed_edges.reserve(build_edges.size());

    for (uint32_t i = 0; i < build_edges.size(); ++i) {
        if (build_edges[i].id != INVALID_ID) {
            NodeID new_from = node_lut[build_edges[i].from_node];
            NodeID new_to = node_lut[build_edges[i].to_node];

            if (new_from != INVALID_ID && new_to != INVALID_ID) {
                edge_lut[i] = static_cast<EdgeID>(packed_edges.size());
                packed_edges.push_back(build_edges[i]);
                
                GraphEdgeData& edge = packed_edges.back();
                edge.id = edge_lut[i];
                edge.from_node = new_from;
                edge.to_node = new_to;
            }
        }
    }

    build_nodes = std::move(packed_nodes);
    build_edges = std::move(packed_edges);

    _remap_ids(node_lut, edge_lut);

    dirty_flags = ALL;
    _rebuild_topology();
}

void IdeamGraphDOD::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    // Overridden by derived classes
}

void IdeamGraphDOD::clear() {
    build_nodes.clear();
    build_edges.clear();
    dirty_flags = ALL;
}

} // namespace ideam::core