#ifndef IDEAM_CORE_MEMORY_GRAPH_H
#define IDEAM_CORE_MEMORY_GRAPH_H

#include "../graphs/ideam_graph.h"
#include "memory_grant.h"
#include "memory_manager.h"
#include <vector>
#include <unordered_map>

namespace ideam::core {

/**
 * MemoryDirtyFlags
 * Extends base GraphDirtyFlags.
 */
enum MemoryDirtyFlags : uint32_t {
    RESOURCES = 1 << 3, // GrantParts or Selections changed
};

/**
 * NodeGrantMetadata
 * Stores offsets into the master grant_registry CSR.
 */
struct NodeGrantMetadata {
    uint32_t grant_offset = 0;
    uint32_t grant_count = 0;
};

/**
 * MemoryGraph
 * A specialized topology that manages persistent MemoryGrants (Leases).
 * Grants remain valid until a MemoryManager rebase or a Selection mutation occurs.
 */
class MemoryGraph : public IdeamGraph {
protected:
    // Parallel to IdeamGraph::nodes.
    std::vector<NodeGrantMetadata> node_metadata;
    
    // Master contiguous block of all GrantParts for all nodes.
    std::vector<GrantPart> grant_registry;

    // Staging area for requirement definitions before baking into the registry.
    std::vector<std::vector<GrantPart>> staging_requirements;

    // The persistent "Leases". Key is NodeID.
    std::unordered_map<NodeID, MemoryGrant*> persistent_grants;

    MemoryManager* manager = nullptr;

    virtual void _bake_requirements();

public:
    MemoryGraph() = default;
    virtual ~MemoryGraph() override { release_all_grants(); }

    void set_manager(MemoryManager* p_manager) { manager = p_manager; }

    /**
     * validate_grants
     * The core maintenance loop. Audits all active nodes and re-acquires
     * grants if they are missing or have been invalidated by the Manager.
     */
    virtual void validate_grants();

    /**
     * get_grant
     * Retrieves the permission slip for a specific node. 
     * Returns nullptr if the node has no requirements or acquisition failed.
     */
    [[nodiscard]] MemoryGrant* get_grant(NodeID p_id) const;

    /**
     * release_all_grants
     * Returns all active leases to the MemoryManager pool.
     */
    void release_all_grants();

    // --- Node API ---
    
    /**
     * set_node_requirements
     * Assigns buffer claims to a node. If requirements change, 
     * any existing grant for this node is immediately released.
     */
    void set_node_requirements(NodeID p_id, const std::vector<GrantPart>& p_parts);

    /**
     * get_node_requirements
     * Direct access to the baked registry for a specific node.
     */
    const GrantPart* get_node_requirements(NodeID p_id, uint32_t& r_count) const;

    // --- Overrides ---
    virtual void on_topology_changed() override;
    void defragment();
    void clear();
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_GRAPH_H