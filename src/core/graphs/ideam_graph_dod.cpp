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
    godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - START");
    if (dirty_flags & (STRUCTURE | CONNECTIONS)) {
        godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - Calling _bake_adjacency...");
        _bake_adjacency();
        godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - _bake_adjacency COMPLETE.");
    }
    
    godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - Calling _sort_kahn_waves...");
    _sort_kahn_waves();
    godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - _sort_kahn_waves COMPLETE.");
    
    dirty_flags = CLEAN;
    godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - Calling virtual on_topology_changed...");
    on_topology_changed();
    
    godot::UtilityFunctions::print("[DOD Tracker]   _rebuild_topology() - END");
}

void IdeamGraphDOD::_bake_adjacency() {
    uint32_t active_edges = 0;
    for (const auto& edge : build_edges) {
        if (edge.id != INVALID_ID) active_edges++;
    }

    _ensure_buffer(input_registry_id, active_edges * sizeof(EdgeID), sizeof(EdgeID));
    _ensure_buffer(output_registry_id, active_edges * sizeof(EdgeID), sizeof(EdgeID));

    MemoryGrantPOD in_grant, out_grant;
    
    std::array<GrantPartPOD, 1> in_req = {{ 
        GrantPartPOD{ 
            .buffer_id = input_registry_id, 
            .element_stride = sizeof(EdgeID),
            .access_mode = BufferAccessMode::WRITE 
        } 
    }};
    if (const MemoryBufferPOD* b = manager->get_buffer(input_registry_id)) {
        in_req[0].selection.mode = SelectionMode::RANGE;
        in_req[0].selection.start_index = 0;
        in_req[0].selection.element_count = b->max_elements;
        in_req[0].selection.capacity = b->max_elements;
    }
    manager->bake_grant(in_grant, in_req);
    
    
    std::array<GrantPartPOD, 1> out_req = {{ 
        GrantPartPOD{ 
            .buffer_id = output_registry_id, 
            .element_stride = sizeof(EdgeID),
            .access_mode = BufferAccessMode::WRITE 
        } 
    }};
    if (const MemoryBufferPOD* b = manager->get_buffer(output_registry_id)) {
        out_req[0].selection.mode = SelectionMode::RANGE;
        out_req[0].selection.start_index = 0;
        out_req[0].selection.element_count = b->max_elements;
        out_req[0].selection.capacity = b->max_elements;
    }
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
    size_t max_wave_transient = 0;
    
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

        // Aggregate Transient Memory requirement for this parallel wave
        size_t current_wave_transient = 0;
        for (NodeID n : current_wave) {
            current_wave_transient += _get_node_transient_requirement(n);
        }
        if (current_wave_transient > max_wave_transient) {
            max_wave_transient = current_wave_transient;
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

    godot::UtilityFunctions::print("[DOD Tracker]   _sort_kahn_waves() - Ensuring wave buffers...");
    _ensure_buffer(wave_node_id, total_node_count * sizeof(NodeID), sizeof(NodeID));
    _ensure_buffer(wave_meta_id, wave_list.size() * sizeof(WaveInfo), sizeof(WaveInfo));

    MemoryGrantPOD n_grant, m_grant;
    std::array<GrantPartPOD, 1> n_req = {{ 
        GrantPartPOD{ 
            .buffer_id = wave_node_id, 
            .element_stride = sizeof(NodeID), // <--- THE FIX
            .access_mode = BufferAccessMode::WRITE 
        } 
    }};
    if (const MemoryBufferPOD* b = manager->get_buffer(wave_node_id)) {
        n_req[0].selection.mode = SelectionMode::RANGE;
        n_req[0].selection.start_index = 0;
        n_req[0].selection.element_count = b->max_elements;
        n_req[0].selection.capacity = b->max_elements;
    }
    manager->bake_grant(n_grant, n_req);
    
    std::array<GrantPartPOD, 1> m_req = {{ 
        GrantPartPOD{ 
            .buffer_id = wave_meta_id, 
            .element_stride = sizeof(WaveInfo), // <--- THE FIX
            .access_mode = BufferAccessMode::WRITE 
        } 
    }};
    if (const MemoryBufferPOD* b = manager->get_buffer(wave_meta_id)) {
        m_req[0].selection.mode = SelectionMode::RANGE;
        m_req[0].selection.start_index = 0;
        m_req[0].selection.element_count = b->max_elements;
        m_req[0].selection.capacity = b->max_elements;
    }
    manager->bake_grant(m_grant, m_req);
    
    // CRITICAL DIAGNOSTIC: These should NOT be 0 anymore!
    godot::UtilityFunctions::print("[DOD Tracker]   _sort_kahn_waves() - n_grant valid elements: ", n_grant.parts[0].selection.element_count);
    godot::UtilityFunctions::print("[DOD Tracker]   _sort_kahn_waves() - m_grant valid elements: ", m_grant.parts[0].selection.element_count);

    auto n_view = _get_paged_view<NodeID, FlatStrategy>(n_grant, 0);
    auto m_view = _get_paged_view<WaveInfo, FlatStrategy>(m_grant, 0);

    // --- PAGED VIEW DIAGNOSTIC ---
    godot::UtilityFunctions::print("--- PAGED VIEW DIAGNOSTIC ---");
    
    // 1. Is the Grant holding the correct stride?
    godot::UtilityFunctions::print("m_grant Stride: ", m_grant.parts[0].element_stride);
    
    // 2. Did the View successfully grab the Page Table pointer?
    godot::UtilityFunctions::print("m_view Page Table Ptr: ", reinterpret_cast<uint64_t>(m_view.page_table));
    
    // 3. If the table exists, does Page 0 point to valid Master Block memory?
    if (m_view.page_table) {
        godot::UtilityFunctions::print("m_view Page[0] Ptr: ", reinterpret_cast<uint64_t>(m_view.page_table[0]));
    }
    godot::UtilityFunctions::print("-----------------------------");

    godot::UtilityFunctions::print("[DOD Tracker]   _sort_kahn_waves() - Writing to Views...");
    uint32_t write_ptr = 0;
    for (uint32_t w = 0; w < wave_list.size(); ++w) {
        m_view[w] = { write_ptr, static_cast<uint32_t>(wave_list[w].size()) };
        for (NodeID id : wave_list[w]) {
            n_view[write_ptr++] = id;
        }
    }
    
    godot::UtilityFunctions::print("[DOD Tracker]   _sort_kahn_waves() - Releasing Grants...");
    manager->release_grant(n_grant);
    manager->release_grant(m_grant);

    // Apply the high-water mark for the Transient Arena
    if (manager && max_wave_transient > 0) {
        manager->ensure_transient_capacity(max_wave_transient);
    }
}

void IdeamGraphDOD::_ensure_buffer(uint32_t& r_id, size_t p_size_bytes, uint32_t p_stride, uint32_t p_alignment) {
    size_t page_size = 4096;
    size_t aligned_size = ((p_size_bytes + page_size - 1) / page_size) * page_size;
    if (aligned_size == 0) aligned_size = page_size;

    if (r_id != 0xFFFFFFFF) {
        MemoryBufferPOD* buf = manager->get_buffer(r_id);
        if (buf && buf->capacity_bytes >= aligned_size) return;
        
        // Zero-copy virtual page expansion
        if (manager->expand_paged_buffer(r_id, aligned_size)) {
            // IMPORTANT: Expand logical bounds when physical capacity grows!
            if (buf) {
                buf->max_elements = aligned_size / p_stride;
                buf->current_count = buf->max_elements; 
            }
            return;
        }
    }
    
    r_id = manager->create_buffer(BufferLayoutType::PAGED, aligned_size, p_alignment);
    manager->configure_paged(r_id, static_cast<uint32_t>(page_size));

    std::vector<ColumnMetadata> col;
    col.push_back({
        0, 0, 0, DataType::CUSTOM, 0, p_stride, p_alignment, 0
    });
    manager->configure_buffer_columns(r_id, col);

    // Force the utility buffer's logical bounds to match its physical capacity.
    // This gives the PagedView permission to write to the entire block.
    MemoryBufferPOD* buf = manager->get_buffer(r_id);
    if (buf) {
        buf->max_elements = aligned_size / p_stride;
        buf->current_count = buf->max_elements; 
    }
}

void IdeamGraphDOD::reserve(size_t p_node_count, size_t p_edge_count) {
    build_nodes.reserve(p_node_count);
    build_edges.reserve(p_edge_count);
}

void IdeamGraphDOD::defragment() {
    godot::UtilityFunctions::print("[DOD Tracker] IdeamGraphDOD::defragment() - START");
    if (build_nodes.empty() && build_edges.empty()) {
        godot::UtilityFunctions::print("[DOD Tracker] defragment() - Graph is empty. Clearing.");
        clear();
        return;
    }

    godot::UtilityFunctions::print("[DOD Tracker] defragment() - Packing Nodes...");
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

    godot::UtilityFunctions::print("[DOD Tracker] defragment() - Packing Edges...");
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

    godot::UtilityFunctions::print("[DOD Tracker] defragment() - Calling Virtual _remap_ids...");
    _remap_ids(node_lut, edge_lut);
    godot::UtilityFunctions::print("[DOD Tracker] defragment() - Virtual _remap_ids COMPLETE.");

    dirty_flags = ALL;
    
    godot::UtilityFunctions::print("[DOD Tracker] defragment() - Calling _rebuild_topology()...");
    _rebuild_topology();
    godot::UtilityFunctions::print("[DOD Tracker] defragment() - _rebuild_topology() COMPLETE.");
    godot::UtilityFunctions::print("[DOD Tracker] IdeamGraphDOD::defragment() - FULLY COMPLETE.");
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