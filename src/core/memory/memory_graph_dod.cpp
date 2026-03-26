#include "memory_graph_dod.h"
#include <cstring>
#include <algorithm>

namespace ideam::core {

MemoryGraphDOD::MemoryGraphDOD(MemoryManagerDOD* p_manager) : IdeamGraphDOD(p_manager) {
    static_assert(MemoryUtilities::is_dod_safe<MemoryNodeMetadata>(), "MemoryNodeMetadata must be POD.");
}

MemoryGraphDOD::~MemoryGraphDOD() {
    release_all_grants();
}

void MemoryGraphDOD::validate_grants() {
    if (!manager) return;

    if (dirty_flags & (RESOURCES | STRUCTURE)) {
        _bake_requirements();
    }

    if (active_grants.size() < build_nodes.size()) {
        active_grants.resize(build_nodes.size());
        selection_metadata.resize(build_nodes.size());
    }

    MemoryGrantPOD reg_grant;
    bool reg_available = false;
    GrantPartPOD* reg_base = nullptr;

    if (registry_buffer_id != INVALID_ID) {
        reg_available = manager->bake_grant(reg_grant, { 
            GrantPartPOD{ .buffer_id = registry_buffer_id, .access_mode = BufferAccessMode::READ } 
        });
        if (reg_available) {
            reg_base = reinterpret_cast<GrantPartPOD*>(reg_grant.parts[0].raw_base_ptr);
        }
    }

    for (uint32_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes[i].id == INVALID_ID) continue;

        MemoryGrantPOD& grant = active_grants[i];

        // 1. Physical Validity Check (Memory Manager Level)
        // If pointers are dead or manager rebased, we must perform a full re-acquisition.
        if (!grant.active || !grant.is_valid()) {
            
            if (grant.active) {
                manager->release_grant(grant);
            }

            const MemoryNodeMetadata& meta = node_metadata[i];
            if (meta.req_count == 0 || !reg_available) continue;

            const GrantPartPOD* node_reqs_start = reg_base + meta.req_offset;
            std::vector<GrantPartPOD> reqs;
            reqs.assign(node_reqs_start, node_reqs_start + meta.req_count);
            
            if (manager->bake_grant(grant, reqs)) {
                // On fresh acquisition, all selections are considered dirty.
                selection_metadata[i].dirty_parts_mask = 0xFF;
            }
        }
        // 2. Logical Selection Check (Graph Level)
        // If physical pointers are fine, but the graph flags indicate a selection shift...
        else if (selection_metadata[i].dirty_parts_mask != 0) {
            // No action needed here; the TaskGraph will observe the mask and execute 
            // the necessary Query Kernels to 'clean' the selection.
        }
    }

    if (reg_available) {
        manager->release_grant(reg_grant);
    }
}

void MemoryGraphDOD::fork_grant(NodeID p_parent_idx, NodeID p_child_idx) {
    if (p_parent_idx >= active_grants.size() || p_child_idx >= active_grants.size()) return;

    // Shallow copy the grant: Child now shares the parent's base pointers.
    active_grants[p_child_idx] = active_grants[p_parent_idx];
    
    // Child starts with a dirty selection so it can refine the parent's result.
    selection_metadata[p_child_idx].dirty_parts_mask = 0xFF;
}

void MemoryGraphDOD::mark_selection_dirty(NodeID p_id, uint32_t p_part_mask) {
    if (p_id < selection_metadata.size()) {
        selection_metadata[p_id].dirty_parts_mask |= static_cast<uint8_t>(p_part_mask);
    }
}

const MemoryGrantPOD* MemoryGraphDOD::get_grant(NodeID p_id) const {
    if (p_id < active_grants.size() && active_grants[p_id].active) {
        return &active_grants[p_id];
    }
    return nullptr;
}

SelectionMetadata* MemoryGraphDOD::get_selection_meta(NodeID p_id) {
    return (p_id < selection_metadata.size()) ? &selection_metadata[p_id] : nullptr;
}

void MemoryGraphDOD::set_node_requirements(NodeID p_id, const std::vector<GrantPartPOD>& p_parts) {
    if (p_id >= build_nodes.size() || build_nodes[p_id].id == INVALID_ID) return;

    if (p_id < active_grants.size() && active_grants[p_id].active) {
        manager->release_grant(active_grants[p_id]);
    }

    if (p_id >= staging_requirements.size()) {
        staging_requirements.resize(build_nodes.size());
    }

    staging_requirements[p_id] = p_parts;
    dirty_flags |= RESOURCES;
}

void MemoryGraphDOD::release_all_grants() {
    if (!manager) return;
    for (auto& grant : active_grants) {
        if (grant.active) {
            manager->release_grant(grant);
        }
    }
    active_grants.clear();
    selection_metadata.clear();
}

void MemoryGraphDOD::on_topology_changed() {
    dirty_flags |= RESOURCES;
}

void MemoryGraphDOD::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    std::vector<std::vector<GrantPartPOD>> packed_staging(build_nodes.size());
    std::vector<MemoryGrantPOD> packed_grants(build_nodes.size());
    std::vector<SelectionMetadata> packed_meta(build_nodes.size());

    for (uint32_t i = 0; i < p_node_lut.size(); ++i) {
        NodeID new_idx = p_node_lut[i];
        if (new_idx != INVALID_ID) {
            if (i < staging_requirements.size()) packed_staging[new_idx] = std::move(staging_requirements[i]);
            if (i < active_grants.size()) packed_grants[new_idx] = active_grants[i];
            if (i < selection_metadata.size()) packed_meta[new_idx] = selection_metadata[i];
        } else {
            if (i < active_grants.size() && active_grants[i].active) {
                manager->release_grant(active_grants[i]);
            }
        }
    }

    staging_requirements = std::move(packed_staging);
    active_grants = std::move(packed_grants);
    selection_metadata = std::move(packed_meta);
    dirty_flags |= RESOURCES;
}

void MemoryGraphDOD::_bake_requirements() {
    uint32_t total_parts = 0;
    for (uint32_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes[i].id != INVALID_ID && i < staging_requirements.size()) {
            total_parts += static_cast<uint32_t>(staging_requirements[i].size());
        }
    }

    _ensure_buffer(registry_buffer_id, total_parts * sizeof(GrantPartPOD));
    
    MemoryGrantPOD reg_write_grant;
    bool bake_ok = manager->bake_grant(reg_write_grant, { 
        GrantPartPOD{ .buffer_id = registry_buffer_id, .access_mode = BufferAccessMode::WRITE } 
    });

    if (!bake_ok) return;

    GrantPartPOD* reg_ptr = reinterpret_cast<GrantPartPOD*>(reg_write_grant.parts[0].raw_base_ptr);
    node_metadata.assign(build_nodes.size(), {0, 0});
    
    uint32_t current_offset = 0;
    for (uint32_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes[i].id == INVALID_ID) continue;

        if (i < staging_requirements.size()) {
            const auto& reqs = staging_requirements[i];
            node_metadata[i].req_offset = current_offset;
            node_metadata[i].req_count = static_cast<uint32_t>(reqs.size());

            if (!reqs.empty()) {
                std::memcpy(reg_ptr + current_offset, reqs.data(), reqs.size() * sizeof(GrantPartPOD));
                current_offset += node_metadata[i].req_count;
            }
        }
    }

    manager->release_grant(reg_write_grant);
    dirty_flags &= ~RESOURCES;
}

void MemoryGraphDOD::_sort_kahn_waves() {
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

    MemoryGrantPOD out_reg_grant;
    bool out_reg_ok = manager->bake_grant(out_reg_grant, {
        GrantPartPOD{ .buffer_id = output_registry_id, .access_mode = BufferAccessMode::READ } 
    });
    EdgeID* out_edge_ptr = out_reg_ok ? reinterpret_cast<EdgeID*>(out_reg_grant.parts[0].raw_base_ptr) : nullptr;

    size_t total_node_count = 0;
    while (!current_wave.empty()) {
        std::sort(current_wave.begin(), current_wave.end(), [this](NodeID a, NodeID b) {
            if (build_nodes[a].execution_priority != build_nodes[b].execution_priority) {
                return build_nodes[a].execution_priority > build_nodes[b].execution_priority;
            }
            return a < b;
        });

        total_node_count += current_wave.size();
        wave_list.push_back(current_wave);
        std::vector<NodeID> next_wave;

        for (NodeID u : current_wave) {
            const GraphNodeData& node = build_nodes[u];
            if (!out_edge_ptr) continue;

            for (uint32_t i = 0; i < node.output_edge_count; ++i) {
                EdgeID eid = out_edge_ptr[node.output_edge_offset + i];
                NodeID v = build_edges[eid].to_node;
                if (--in_degree[v] == 0) next_wave.push_back(v);
            }
        }
        current_wave = std::move(next_wave);
    }

    if (out_reg_ok) manager->release_grant(out_reg_grant);

    _ensure_buffer(wave_node_id, total_node_count * sizeof(NodeID));
    _ensure_buffer(wave_meta_id, wave_list.size() * sizeof(WaveInfo));

    MemoryGrantPOD n_grant, m_grant;
    manager->bake_grant(n_grant, { GrantPartPOD{ .buffer_id = wave_node_id, .access_mode = BufferAccessMode::WRITE } });
    manager->bake_grant(m_grant, { GrantPartPOD{ .buffer_id = wave_meta_id, .access_mode = BufferAccessMode::WRITE } });
    
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

void MemoryGraphDOD::clear() {
    release_all_grants();
    node_metadata.clear();
    selection_metadata.clear();
    staging_requirements.clear();
    IdeamGraphDOD::clear();
}

} // namespace ideam::core