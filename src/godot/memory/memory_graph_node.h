#ifndef IDEAM_GODOT_MEMORY_GRAPH_NODE_H
#define IDEAM_GODOT_MEMORY_GRAPH_NODE_H

#include "../graphs/ideam_graph_node.h"
#include "memory_inspectors.h"
#include <godot_cpp/classes/button.hpp>
#include <unordered_map>

namespace godot {

/**
 * MemoryGraphPortTraits
 * Bitmask defining the capability/strategy required or provided by a specific memory graph port.
 * Evaluated at runtime by the Editor UI to prevent invalid topological connections.
 */
enum MemoryGraphPortTraits : uint32_t {
    TRAIT_NONE           = 0,
    TRAIT_LINEAR_ACCESS  = 1 << 0, 
    TRAIT_SPATIAL_ACCESS = 1 << 1, 
    TRAIT_SIMD_ACCESS    = 1 << 2, 
    TRAIT_RANDOM_ACCESS  = 1 << 3, 
    TRAIT_VIRTUAL_MEMORY = 1 << 4,
    TRAIT_IS_SPATIAL     = 1 << 5, 
    TRAIT_IS_PAGED       = 1 << 6  
};

class MemoryGraphNode : public IdeamGraphNode {
    GDCLASS(MemoryGraphNode, IdeamGraphNode)

private:
    // Telemetry Snapshot
    Ref<ideam::godot_ext::MemoryGrantInspector> latest_grant_snapshot;

    // UI Elements
    Button* inspect_memory_btn = nullptr;

protected:
    // Maps port_index -> Trait Bitmask
    std::unordered_map<int, uint32_t> input_port_signatures;
    std::unordered_map<int, uint32_t> output_port_signatures;

    static void _bind_methods();
    virtual void _build_ui() override;

    void _on_inspect_memory_pressed();

    /**
     * @brief Helper for derived classes to declare their compile-time traits to the UI.
     * Should be called inside overridden _build_ui().
     */
    void register_port_signature(int p_port_idx, bool p_is_output, uint32_t p_trait_mask);

public:
    MemoryGraphNode();
    virtual ~MemoryGraphNode() override = default;

    /**
     * @brief Pushes a runtime snapshot of the node's memory lease to the UI.
     */
    void update_telemetry(const Ref<ideam::godot_ext::MemoryGrantInspector>& p_inspector);

    /**
     * @brief API for the GraphEdit to query when the user drags a connection line.
     */
    uint32_t get_port_signature(int p_port_idx, bool p_is_output) const;
};

} // namespace godot

VARIANT_BITFIELD_CAST(godot::MemoryGraphPortTraits);

#endif // IDEAM_GODOT_MEMORY_GRAPH_NODE_H