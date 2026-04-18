#pragma once

#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include "ideam_graph_node.h"
#include "ideam_graph_resource.h" 

namespace ideam::godot_ext {

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

    void _on_node_property_changed(const godot::StringName& p_node_name, const godot::StringName& p_property_name, const godot::Variant& p_new_value);
    void _on_node_delete_request(const godot::StringName& p_node_name);

    // UI Signal Handlers
    void _request_connect(const godot::StringName &p_from_node, int p_from_port, const godot::StringName &p_to_node, int p_to_port);
    void _request_disconnect(const godot::StringName &p_from_node, int p_from_port, const godot::StringName &p_to_node, int p_to_port);
    void _show_popup(const godot::Vector2 &p_at);
    void _popup_select(int p_id);
    
    // Triggered when the user finishes dragging a node
    void _on_end_node_move();

    // The Reactive Update Loop
    void _on_blueprint_changed();

    // Internal Helpers
    void _create_popup();
    virtual godot::TypedArray<godot::String> _get_new_node_types() const;
    virtual void _spawn_node_by_type(int p_type_id);

public:
    IdeamGraphEdit();
    virtual ~IdeamGraphEdit() override;

    void _ready() override;

    // --- Blueprint Integration ---
    void set_blueprint(const godot::Ref<IdeamGraphResource>& p_blueprint);
    godot::Ref<IdeamGraphResource> get_blueprint() const { return current_blueprint; }

    void node_context_clicked(IdeamGraphNode* p_node);
};

} // namespace ideam::godot_ext

 // IDEAM_GRAPH_EDIT_H