#pragma once

#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "ideam_graph_node.h"
#include "ideam_graph_resource.h" 

namespace ideam::godot_ext {

/**
 * @class IdeamGraphEdit
 * @brief The presentation and mutation-routing layer for the DOD graph topology.
 * Enforces structural access rules and routes all user commands directly 
 * to the underlying memory graph resource before visually reflecting changes.
 */
class IdeamGraphEdit : public godot::GraphEdit {
    GDCLASS(IdeamGraphEdit, godot::GraphEdit)

protected:
    // The definitive source of truth for this graph's layout
    godot::Ref<IdeamGraphResource> current_blueprint;
    
    // UI Elements
    godot::PopupMenu* context_popup = nullptr;
    godot::Vector2 popup_position;
    
    // Selection state for context actions
    IdeamGraphNode* context_node = nullptr;

    // Internal sync lock to prevent infinite feedback loops when dragging nodes
    bool is_syncing_ui = false;

    static void _bind_methods();

    virtual void _notification(int p_what);
    virtual void _update_theme_properties();
    void _on_node_property_changed(const godot::StringName& p_node_name, const godot::StringName& p_property_name, const godot::Variant& p_new_value);
    void _on_node_delete_request(const godot::StringName& p_node_name);
    void _on_node_connections_requested(godot::Object* p_node);
    
    // UI Signal Handlers
    void _request_connect(const godot::StringName &p_from_node, int p_from_port, const godot::StringName &p_to_node, int p_to_port);
    void _request_disconnect(const godot::StringName &p_from_node, int p_from_port, const godot::StringName &p_to_node, int p_to_port);
    void _show_popup(const godot::Vector2 &p_at);
    void _popup_select(int p_id);
    void _frame_attached(const godot::StringName& p_element, const godot::StringName& p_frame);
    void _frame_detached(const godot::StringName& p_element, const godot::StringName& p_frame);
    
    // Triggered when the user finishes dragging a node
    void _on_end_node_move();

    // The Reactive Update Loop
    void _on_blueprint_changed();

    // Internal Helpers
    void _create_popup();
    virtual IdeamGraphNode* _create_graph_node(const godot::Ref<IdeamGraphNodeResource>& p_node_res);
    
    virtual godot::TypedArray<godot::String> _get_new_node_types() const;
    virtual void _spawn_node_by_type(int p_type_id);

public:
    IdeamGraphEdit();
    virtual ~IdeamGraphEdit() override;

    void _ready() override;
    
    // Public entry point for nodes to request a context menu
    void node_context_clicked(godot::Object* p_node);

    // Resource Injection
    void set_blueprint(const godot::Ref<IdeamGraphResource>& p_blueprint);
    godot::Ref<IdeamGraphResource> get_blueprint() const { return current_blueprint; }
};

} // namespace ideam::godot_ext