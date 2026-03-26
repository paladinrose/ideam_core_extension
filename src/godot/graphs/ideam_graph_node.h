#ifndef IDEAM_GRAPH_NODE_H
#define IDEAM_GRAPH_NODE_H

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include "../../core/graphs/ideam_graph.h"

namespace godot {

class IdeamGraphNode : public GraphNode {
    GDCLASS(IdeamGraphNode, GraphNode)

private:
    // Link to the specific node in the Core Topology
    ideam::core::NodeID core_node_id = ideam::core::INVALID_NODE;

protected:
    static void _bind_methods();

public:
    IdeamGraphNode();
    virtual ~IdeamGraphNode() override;

    void _ready() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

    // --- Core Sync ---
    void set_core_node_id(ideam::core::NodeID p_id) { core_node_id = p_id; }
    ideam::core::NodeID get_core_node_id() const { return core_node_id; }

    // --- Context Menu Hooks ---
    // Child classes (e.g., TaskNode) override these to populate options
    virtual TypedArray<String> get_context_menu_options() const;
    virtual void select_context_menu_option(int p_id);

    // Helper to notify the parent GraphEdit
    void _emit_context_request();
};

} // namespace godot

#endif // IDEAM_GRAPH_NODE_H