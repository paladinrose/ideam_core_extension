#pragma once

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/color.hpp>
#include <map>

namespace ideam::godot_ext {

/**
 * @class IdeamGraphNode
 * @brief The visual UI component representing a node in the DOD graph.
 * This class acts strictly as a View. It reads from a state dictionary, 
 * reflects the underlying DOD resource topology, and emits signals 
 * when the user requests a mutation.
 */
class IdeamGraphNode : public godot::GraphNode {
    GDCLASS(IdeamGraphNode, godot::GraphNode)

public:
    enum NodeFrameState {
        FRAME_DEFAULT,
        FRAME_SELECTED,
        FRAME_CONTEXT_HOVER,
        FRAME_LOCKED,
        FRAME_ERROR
    };

    enum PortState {
        PORT_EMPTY,
        PORT_CONNECTED,
        PORT_LOCKED,
        PORT_ERROR
    };

private:
    // The unique ID linking this UI node to its entry in the IdeamGraphResource
    godot::StringName blueprint_id;
    
    // The DOD classification type (determines ports and UI)
    uint32_t type_id = 0;

    // Cached state of node-specific properties
    godot::Dictionary properties;

    // Structural states tracking DOD enforcement
    bool is_locked_state = false;
    bool is_error_state = false;
    bool is_context_hovered = false;

    // Internal trackers for logical port access control
    std::map<int, PortState> left_port_states;
    std::map<int, PortState> right_port_states;

    // Helper to recursively disable internal UI controls when locked
    void _set_controls_disabled(godot::Node* p_node, bool p_disabled);
    
    // Helper to extract the proper theme color based on logical port state
    godot::Color _get_color_for_port_state(PortState p_state) const;

protected:
    static void _bind_methods();

    void _notification(int p_what);

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
    void emit_property_changed(const godot::StringName& p_property_name, const godot::Variant& p_new_value);

public:
    IdeamGraphNode();
    virtual ~IdeamGraphNode() override;

    void _ready() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    // --- State Synchronization ---
    /**
     * @brief Initializes or updates the node's visual state from the Resource dictionary.
     */
    void initialize(const godot::Dictionary& p_node_data);

    godot::StringName get_blueprint_id() const { return blueprint_id; }
    uint32_t get_type_id() const { return type_id; }
    godot::Dictionary get_properties() const { return properties; }

    // --- Interaction & Structural Invalidation Setters ---
    void set_locked(bool p_locked);
    void set_error_state(bool p_error);
    void set_context_hover(bool p_hovered);

    // --- Strict Port Access Control ---
    void update_port_state(int p_slot_index, bool p_is_left, PortState p_state);

    // --- Context Menu Hooks ---
    // Child classes (e.g., TransformTaskNode) override these to populate specific right-click options.
    virtual godot::TypedArray<godot::String> get_context_menu_options() const;
    virtual void select_context_menu_option(int p_option_id);
};

} // namespace ideam::godot_ext