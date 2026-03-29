#ifndef IDEAM_CORE_MEMORY_GRAPH_DOD_H
#define IDEAM_CORE_MEMORY_GRAPH_DOD_H

#include "../graphs/ideam_graph_dod.h"
#include "memory_manager_dod.h"
#include <vector>
#include <span>

namespace ideam::core {

/**
 * MemoryNodeMetadata
 * Parallel to IdeamGraphDOD::build_nodes.
 * Stores the location of node requirements in the contiguous registry.
 */
struct MemoryNodeMetadata {
    uint32_t req_offset = 0;
    uint32_t req_count = 0;
};

/**
 * StagingReqMetadata
 * Tracks append-only positions in the staging data vector before baking.
 */
struct StagingReqMetadata {
    uint32_t offset = 0;
    uint32_t count = 0;
};

/**
 * SelectionMetadata
 * Tracks the reactive state of a node's memory selections.
 */
struct SelectionMetadata {
    // Bitmask: Bit i is 1 if GrantPart i's selection is dirty.
    uint8_t dirty_parts_mask = 0;
    
    // Memoization: Snapshot of external state (e.g., Camera Version) 
    // to skip query execution.
    uint64_t dependency_version = 0;
};

/**
 * MemoryGraphDOD
 * A specialized topology that manages persistent MemoryGrants (Leases).
 * Supports Selection-Aware forking and reactive memoization.
 */
class MemoryGraphDOD : public IdeamGraphDOD {
protected:
    // --- Build Phase Storage (DOD Append-Only Log) ---
    // Replaced fragmented vector-of-vectors with a contiguous CSR-style staging log.
    std::vector<StagingReqMetadata> staging_meta;
    std::vector<GrantPartPOD> staging_data;
    
    // --- Execution Phase Data ---
    uint32_t registry_buffer_id = INVALID_ID;
    
    // Parallel to build_nodes
    std::vector<MemoryNodeMetadata> node_metadata;
    std::vector<SelectionMetadata> selection_metadata;
    std::vector<MemoryGrantPOD> active_grants;

    // Internal Custom Dirty Flag
    static constexpr uint32_t RESOURCES = 1 << 3;

    // Internal Helpers
    void _bake_requirements();
    
    // --- Overrides ---
    virtual void _remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) override;
    virtual void on_topology_changed() override;

public:
    explicit MemoryGraphDOD(MemoryManagerDOD* p_manager);
    virtual ~MemoryGraphDOD() override;

    /**
     * validate_grants
     * Audits leases. Handles physical re-acquisition (manager-side) 
     * and logical selection-dirtying (graph-side) across Virtual Pages.
     */
    void validate_grants();

    /**
     * fork_grant
     * Inherits memory pointers from a parent but allows independent selection shaping.
     * Use this for sequential filtering (e.g., Directional -> Frustum).
     */
    void fork_grant(NodeID p_parent_idx, NodeID p_child_idx);

    /**
     * mark_selection_dirty
     * Manually flag a part of a node's selection as needing a re-query.
     */
    void mark_selection_dirty(NodeID p_id, uint32_t p_part_mask = 0xFF);

    /**
     * set_node_requirements
     * Appends node requirements to the staging log using zero-copy C++20 spans.
     */
    void set_node_requirements(NodeID p_id, std::span<const GrantPartPOD> p_parts);

    [[nodiscard]] const MemoryGrantPOD* get_grant(NodeID p_id) const;
    [[nodiscard]] SelectionMetadata* get_selection_meta(NodeID p_id);

    void release_all_grants();
    void clear();
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_GRAPH_DOD_H