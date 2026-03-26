#include "memory_graph.h"
#include <algorithm>

namespace ideam::core {

void MemoryGraph::validate_grants() {
    if (!manager) return;

    // 1. Structural/Resource Bake
    if (dirty_flags & (RESOURCES | STRUCTURE)) {
        _bake_requirements();
    }

    // 2. Lease Audit
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id == INVALID_ID) continue;

        MemoryGrant* current_lease = nullptr;
        if (persistent_grants.count(i)) {
            current_lease = persistent_grants[i];
        }

        // Re-acquisition triggers if:
        // - Lease is missing
        // - Lease is invalid (Manager rebased memory or Selection version bumped)
        if (!current_lease || !current_lease->is_valid()) {
            
            if (current_lease) {
                manager->release_grant(current_lease);
                persistent_grants.erase(i);
            }

            uint32_t count = 0;
            const GrantPart* parts_ptr = get_node_requirements(i, count);
            
            if (count > 0) {
                // Request new lease from authority
                std::vector<GrantPart> reqs(parts_ptr, parts_ptr + count);
                MemoryGrant* new_grant = manager->request_grant(reqs);
                
                if (new_grant) {
                    persistent_grants[i] = new_grant;
                }
            }
        }
    }
}

MemoryGrant* MemoryGraph::get_grant(NodeID p_id) const {
    auto it = persistent_grants.find(p_id);
    if (it != persistent_grants.end()) {
        return it->second;
    }
    return nullptr;
}

void MemoryGraph::set_node_requirements(NodeID p_id, const std::vector<GrantPart>& p_parts) {
    if (p_id >= nodes.size() || nodes[p_id].id == INVALID_ID) return;

    // Invalidate existing lease immediately if requirements change
    if (persistent_grants.count(p_id)) {
        manager->release_grant(persistent_grants[p_id]);
        persistent_grants.erase(p_id);
    }

    if (p_id >= staging_requirements.size()) {
        staging_requirements.resize(nodes.size());
    }

    staging_requirements[p_id] = p_parts;
    dirty_flags |= RESOURCES;
}

const GrantPart* MemoryGraph::get_node_requirements(NodeID p_id, uint32_t& r_count) const {
    if (p_id >= node_metadata.size() || (p_id < nodes.size() && nodes[p_id].id == INVALID_ID)) {
        r_count = 0;
        return nullptr;
    }
    r_count = node_metadata[p_id].grant_count;
    return &grant_registry[node_metadata[p_id].grant_offset];
}

void MemoryGraph::release_all_grants() {
    if (!manager) return;
    for (auto const& [id, grant] : persistent_grants) {
        manager->release_grant(grant);
    }
    persistent_grants.clear();
}

void MemoryGraph::on_topology_changed() {
    // Topology shifts invalidate the CSR registry and potentially all leases
    dirty_flags |= RESOURCES;
}

void MemoryGraph::defragment() {
    if (nodes.empty()) {
        clear();
        return;
    }

    // Capture LUT before parent collapses vectors
    std::vector<NodeID> node_lut(nodes.size(), INVALID_ID);
    uint32_t next_valid_idx = 0;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id != INVALID_ID) {
            node_lut[i] = next_valid_idx++;
        }
    }

    // Sync staging
    std::vector<std::vector<GrantPart>> new_staging(next_valid_idx);
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (node_lut[i] != INVALID_ID) {
            new_staging[node_lut[i]] = std::move(staging_requirements[i]);
        }
    }
    staging_requirements = std::move(new_staging);

    // Release all grants because indices have fundamentally shifted
    release_all_grants();

    IdeamGraph::defragment();
    dirty_flags |= RESOURCES;
}

void MemoryGraph::_bake_requirements() {
    if (staging_requirements.size() < nodes.size()) {
        staging_requirements.resize(nodes.size());
    }

    node_metadata.assign(nodes.size(), {0, 0});
    
    uint32_t total_parts = 0;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id != INVALID_ID) {
            total_parts += static_cast<uint32_t>(staging_requirements[i].size());
        }
    }
    
    grant_registry.clear();
    grant_registry.reserve(total_parts);

    uint32_t current_offset = 0;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id == INVALID_ID) continue;

        const auto& parts = staging_requirements[i];
        node_metadata[i].grant_offset = current_offset;
        node_metadata[i].grant_count = static_cast<uint32_t>(parts.size());

        for (const auto& part : parts) {
            grant_registry.push_back(part);
        }
        
        current_offset += node_metadata[i].grant_count;
    }
    
    dirty_flags &= ~RESOURCES;
}

void MemoryGraph::clear() {
    release_all_grants();
    IdeamGraph::clear();
    node_metadata.clear();
    grant_registry.clear();
    staging_requirements.clear();
}

} // namespace ideam::core