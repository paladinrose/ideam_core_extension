#ifndef IDEAM_GRAPH_NODE_H
#define IDEAM_GRAPH_NODE_H

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

/**
 * @class IdeamGraphNode
 * @brief The visual UI component representing a node in the DOD graph.
 * This class is purely a View. It reads from a state dictionary and emits signals 
 * when the user requests a mutation.
 */
class IdeamGraphNode : public GraphNode {
    GDCLASS(IdeamGraphNode, GraphNode)

private:
    // The unique ID linking this UI node to its entry in the IdeamGraphResource
    StringName blueprint_id;
    
    // The DOD classification type (determines ports and UI)
    uint32_t type_id = 0;

    // Cached state of node-specific properties
    Dictionary properties;

protected:
    static void _bind_methods();

    // Internal helper to route context menus
    void _emit_context_request();

    /**
     * @brief Virtual method for derived classes to generate their specific ports and UI fields.
     * Called automatically at the end of initialize().
     */
    virtual void _build_ui();

    /**
     * @brief Helper for derived nodes to notify the parent GraphEdit that a property was changed by the user.
     */
    void emit_property_changed(const StringName& p_property_name, const Variant& p_new_value);

public:
    IdeamGraphNode();
    virtual ~IdeamGraphNode() override;

    void _ready() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

    // --- State Synchronization ---
    /**
     * @brief Initializes or updates the node's visual state from the Resource dictionary.
     */
    void initialize(const Dictionary& p_node_data);

    StringName get_blueprint_id() const { return blueprint_id; }
    uint32_t get_type_id() const { return type_id; }
    Dictionary get_properties() const { return properties; }

    // --- Context Menu Hooks ---
    // Child classes (e.g., TransformTaskNode) override these to populate specific right-click options.
    virtual TypedArray<String> get_context_menu_options() const;
    virtual void select_context_menu_option(int p_id);
};

} // namespace godot

#endif // IDEAM_GRAPH_NODE_H