#ifndef IDEAM_GRAPH_EDIT_H
#define IDEAM_GRAPH_EDIT_H

#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include "ideam_graph_node.h"

// Forward declare our DOD Resource blueprint
namespace ideam::godot_ext {
    class IdeamGraphResource;
}

namespace godot {

class IdeamGraphEdit : public GraphEdit {
    GDCLASS(IdeamGraphEdit, GraphEdit)

private:
    // The definitive source of truth for this graph's layout
    Ref<ideam::godot_ext::IdeamGraphResource> current_blueprint;

    // UI Elements
    PopupMenu* context_popup = nullptr;
    Vector2 popup_position;
    
    // Selection state for context actions
    IdeamGraphNode* context_node = nullptr;

    // Internal sync lock to prevent infinite feedback loops when dragging nodes
    bool is_syncing_ui = false;

protected:
    static void _bind_methods();

    // UI Signal Handlers
    void _request_connect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port);
    void _request_disconnect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port);
    void _show_popup(const Vector2 &p_at);
    void _popup_select(int p_id);
    
    // Triggered when the user finishes dragging a node
    void _on_end_node_move();

    // The Reactive Update Loop
    void _on_blueprint_changed();

    // Internal Helpers
    void _create_popup();
    virtual TypedArray<String> _get_new_node_types() const;
    virtual void _spawn_node_by_type(int p_type_id);

public:
    IdeamGraphEdit();
    virtual ~IdeamGraphEdit() override;

    void _ready() override;

    // --- Blueprint Integration ---
    void set_blueprint(const Ref<ideam::godot_ext::IdeamGraphResource>& p_blueprint);
    Ref<ideam::godot_ext::IdeamGraphResource> get_blueprint() const { return current_blueprint; }

    void node_context_clicked(IdeamGraphNode* p_node);
};

} // namespace godot

#endif // IDEAM_GRAPH_EDIT_H