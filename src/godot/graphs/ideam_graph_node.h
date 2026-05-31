#pragma once

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>

#include <map>

#include "ideam_graph_node_resource.h"

namespace ideam::godot_ext {

/**
 * @class IdeamGraphNode
 * @brief The visual UI component representing a node in the DOD graph.
 * This class acts strictly as a View. It reads directly from its strongly-typed 
 * Authoring Resource, reflecting the underlying DOD topology without caching state locally.
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

protected:
    // Strong 1:1 Reference to the Authoring Model
    godot::Ref<IdeamGraphNodeResource> node_resource;

    // Structural states tracking DOD enforcement
    bool is_locked_state = false;
    bool is_error_state = false;
    bool is_context_hovered = false;
    bool is_updating_theme = false;
    
    bool ui_built = false;

    // Internal trackers for logical port access control
    std::map<int, PortState> left_port_states;
    std::map<int, PortState> right_port_states;

    godot::HBoxContainer* badge_container = nullptr;
    godot::Button* lock_btn = nullptr;

    void _on_lock_toggled();

    // Helper to recursively disable internal UI controls when locked
    void _set_controls_disabled(godot::Node* p_node, bool p_disabled);
    
    // Helper to extract the proper theme color based on logical port state
    godot::Color _get_color_for_port_state(PortState p_state) const;

    static void _bind_methods();

    // Internal helper to route context menus
    void _emit_context_request();

    /**
     * @brief Virtual method for derived classes to generate their specific ports and UI fields.
     * Called automatically at the end of initialize().
     */
    virtual void _build_ui();
    virtual void _update_theme_properties();
    
    void _notification(int p_what);

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
     * @brief Initializes or updates the node's visual state directly from the strong Resource type.
     */
    void initialize(const godot::Ref<IdeamGraphNodeResource>& p_node_res);

    /**
     * @brief Updates only mutable data-driven properties without altering structural child components.
     */
    void update_from_resource(const godot::Ref<IdeamGraphNodeResource>& p_node_res);

    // Direct access to the authoritative model
    godot::Ref<IdeamGraphNodeResource> get_node_resource() const { return node_resource; }
    
    // Convenience wrapper for the core identity string
    godot::StringName get_blueprint_id() const;

    // --- Interaction & Structural Invalidation Setters ---
    void set_locked(bool p_locked);
    bool get_locked() const;

    void set_error_state(bool p_error);
    bool get_error_state() const;
    
    void set_context_hover(bool p_hovered);
    bool get_context_hover() const;

    void add_badge(godot::Control* badge);
    void remove_badge(godot::Control* badge);
    void clear_badges();

    // --- Strict Port Access Control ---
    void update_port_state(int p_slot_index, bool p_is_left, PortState p_state);

    virtual void receive_connection_info(const godot::Dictionary& p_info);
    void request_connections();
    
    // --- Context Menu Hooks ---
    // Child classes (e.g., TransformTaskNode) override these to populate specific right-click options.
    virtual godot::TypedArray<godot::String> get_context_menu_options() const;
    virtual void select_context_menu_option(int p_option_id);
};

} // namespace ideam::godot_ext