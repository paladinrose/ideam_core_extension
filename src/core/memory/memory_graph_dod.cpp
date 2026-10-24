#include "memory_graph_dod.h"
#include <cstring>
#include <algorithm>
#include <array>

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
    if (fork_parents.size() < build_nodes.size()) {
        fork_parents.resize(build_nodes.size(), INVALID_ID);
    }

    MemoryGrantPOD reg_grant;
    bool reg_available = false;

    if (registry_buffer_id != INVALID_ID) {
        std::array<GrantPartPOD, 1> reg_req = {{ 
            GrantPartPOD{ 
                .buffer_id = registry_buffer_id, 
                .element_stride = sizeof(GrantPartPOD),
                .access_mode = BufferAccessMode::READ 
            } 
        }};
        if (const MemoryBufferPOD* b = manager->get_buffer(registry_buffer_id)) {
            reg_req[0].selection.mode = SelectionMode::RANGE;
            reg_req[0].selection.start_index = 0;
            reg_req[0].selection.element_count = b->max_elements;
            reg_req[0].selection.capacity = b->max_elements;
        }
        reg_available = manager->bake_grant(reg_grant, reg_req);
    }

    // Branchless hardware-aware extraction via PagedView
    auto reg_view = reg_available ? _get_paged_view<GrantPartPOD, FlatStrategy>(reg_grant, 0) : PagedView<GrantPartPOD, FlatStrategy>{};

    bool any_reacquired = false;

    for (size_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes.id[i] == INVALID_ID) continue;

        // Skip forked nodes in the primary pass; they inherit from their parent below
        if (i < fork_parents.size() && fork_parents[i] != INVALID_ID) continue;

        MemoryGrantPOD& grant = active_grants[i];

        // 1. Physical Validity Check (Memory Manager Level)
        if (!grant.active || !grant.is_valid()) {
            if (grant.active) {
                manager->release_grant(grant);
            }

            if (i >= node_metadata.size()) continue; 
            const MemoryNodeMetadata& meta = node_metadata[i];
            
            if (meta.req_count > 0 && !reg_available) continue;

            std::vector<GrantPartPOD> reqs;
            if (meta.req_count > 0) {
                reqs.resize(meta.req_count);
                for (uint32_t r = 0; r < meta.req_count; ++r) {
                    reqs[r] = reg_view[meta.req_offset + r];
                }
            }
            
            if (manager->bake_grant(grant, reqs)) {
                selection_metadata[i].dirty_parts_mask = 0xFF;
                any_reacquired = true;
            }
        }
    }

    // Re-propagate forked grants in topological/index order if any parent grant was re-baked or child is invalid
    for (size_t pass = 0; pass < build_nodes.size(); ++pass) {
        bool changed = false;
        for (size_t i = 0; i < build_nodes.size(); ++i) {
            if (build_nodes.id[i] == INVALID_ID || i >= fork_parents.size()) continue;
            NodeID parent = fork_parents[i];
            if (parent == INVALID_ID || parent >= active_grants.size()) continue;

            if (active_grants[parent].active &&
                (any_reacquired || !active_grants[i].active || !active_grants[i].is_valid() ||
                 active_grants[i].part_count != active_grants[parent].part_count ||
                 active_grants[i].manager_version_at_issue != active_grants[parent].manager_version_at_issue)) {
                active_grants[i] = active_grants[parent];
                selection_metadata[i].dirty_parts_mask = 0xFF;
                changed = true;
            }
        }
        if (!changed) break;
    }

    if (reg_available) {
        manager->release_grant(reg_grant);
    }
}

void MemoryGraphDOD::fork_grant(NodeID p_parent_idx, NodeID p_child_idx) {
    if (p_parent_idx >= active_grants.size() || p_child_idx >= active_grants.size()) return;
    if (fork_parents.size() < active_grants.size()) {
        fork_parents.resize(active_grants.size(), INVALID_ID);
    }

    fork_parents[p_child_idx] = p_parent_idx;

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

void MemoryGraphDOD::set_node_requirements(NodeID p_id, std::span<const GrantPartPOD> p_parts) {
    if (p_id >= build_nodes.size() || build_nodes.id[p_id] == INVALID_ID) return;

    if (p_id < active_grants.size() && active_grants[p_id].active) {
        manager->release_grant(active_grants[p_id]);
    }

    if (p_id >= staging_meta.size()) {
        staging_meta.resize(build_nodes.size());
    }

    // DOD Append-Only Staging: Pushes strictly to the back of the flat data vector
    staging_meta[p_id].offset = static_cast<uint32_t>(staging_data.size());
    staging_meta[p_id].count = static_cast<uint32_t>(p_parts.size());
    
    staging_data.insert(staging_data.end(), p_parts.begin(), p_parts.end());
    
    dirty_flags |= RESOURCES;
}

void MemoryGraphDOD::release_all_grants() {
    if (!manager) return;
    for (size_t i = 0; i < active_grants.size(); ++i) {
        const bool is_forked = (i < fork_parents.size() && fork_parents[i] != INVALID_ID);
        if (active_grants[i].active && !is_forked) {
            manager->release_grant(active_grants[i]);
        }
        active_grants[i].active = false;
    }
    active_grants.clear();
    selection_metadata.clear();
    fork_parents.clear();
}

void MemoryGraphDOD::on_topology_changed() {
    dirty_flags |= RESOURCES;
}

void MemoryGraphDOD::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    std::vector<StagingReqMetadata> packed_staging_meta(build_nodes.size());
    std::vector<GrantPartPOD> packed_staging_data;
    packed_staging_data.reserve(staging_data.size());
    
    std::vector<MemoryGrantPOD> packed_grants(build_nodes.size());
    std::vector<SelectionMetadata> packed_meta(build_nodes.size());
    std::vector<NodeID> packed_fork_parents(build_nodes.size(), INVALID_ID);

    for (uint32_t i = 0; i < p_node_lut.size(); ++i) {
        NodeID new_idx = p_node_lut[i];
        if (new_idx != INVALID_ID) {
            if (i < staging_meta.size() && staging_meta[i].count > 0) {
                packed_staging_meta[new_idx].offset = static_cast<uint32_t>(packed_staging_data.size());
                packed_staging_meta[new_idx].count = staging_meta[i].count;
                
                auto start_it = staging_data.begin() + staging_meta[i].offset;
                packed_staging_data.insert(packed_staging_data.end(), start_it, start_it + staging_meta[i].count);
            }
            if (i < active_grants.size()) packed_grants[new_idx] = active_grants[i];
            if (i < selection_metadata.size()) packed_meta[new_idx] = selection_metadata[i];
            if (i < fork_parents.size() && fork_parents[i] != INVALID_ID && fork_parents[i] < p_node_lut.size()) {
                packed_fork_parents[new_idx] = p_node_lut[fork_parents[i]];
            }
        } else {
            if (i < active_grants.size() && active_grants[i].active) {
                manager->release_grant(active_grants[i]);
            }
        }
    }

    staging_meta = std::move(packed_staging_meta);
    staging_data = std::move(packed_staging_data);
    active_grants = std::move(packed_grants);
    selection_metadata = std::move(packed_meta);
    fork_parents = std::move(packed_fork_parents);
    dirty_flags |= RESOURCES;
}

void MemoryGraphDOD::_bake_requirements() {
    uint32_t total_parts = 0;
    for (size_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes.id[i] != INVALID_ID && i < staging_meta.size()) {
            total_parts += staging_meta[i].count;
        }
    }

    _ensure_buffer(registry_buffer_id, total_parts * sizeof(GrantPartPOD), sizeof(GrantPartPOD));
    
    MemoryGrantPOD reg_write_grant;
    std::array<GrantPartPOD, 1> reg_req = {{ 
        GrantPartPOD{ 
            .buffer_id = registry_buffer_id, 
            .element_stride = sizeof(GrantPartPOD),
            .access_mode = BufferAccessMode::WRITE 
        } 
    }};
    
    if (const MemoryBufferPOD* b = manager->get_buffer(registry_buffer_id)) {
        reg_req[0].selection.mode = SelectionMode::RANGE;
        reg_req[0].selection.start_index = 0;
        reg_req[0].selection.element_count = b->max_elements;
        reg_req[0].selection.capacity = b->max_elements;
    }

    if (!manager->bake_grant(reg_write_grant, reg_req)) {
        godot::UtilityFunctions::printerr("[DOD Tracker]     _bake_requirements() - WARNING: bake_grant FAILED. Bailing out.");
        return;
    }

    auto reg_view = _get_paged_view<GrantPartPOD, FlatStrategy>(reg_write_grant, 0);
    node_metadata.assign(build_nodes.size(), {0, 0});
    
    uint32_t current_offset = 0;
    for (size_t i = 0; i < build_nodes.size(); ++i) {
        if (build_nodes.id[i] == INVALID_ID) continue;

        if (i < staging_meta.size() && staging_meta[i].count > 0) {
            node_metadata[i].req_offset = current_offset;
            node_metadata[i].req_count = staging_meta[i].count;

            for (uint32_t r = 0; r < staging_meta[i].count; ++r) {
                reg_view[current_offset++] = staging_data[staging_meta[i].offset + r];
            }
        }
    }

    manager->release_grant(reg_write_grant);
    dirty_flags &= ~RESOURCES;
}

void MemoryGraphDOD::clear() {
    release_all_grants();
    node_metadata.clear();
    selection_metadata.clear();
    staging_meta.clear();
    staging_data.clear();
    IdeamGraphDOD::clear();
}

} // namespace ideam::core