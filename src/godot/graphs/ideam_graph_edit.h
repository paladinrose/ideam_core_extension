#ifndef IDEAM_GRAPH_EDIT_H
#define IDEAM_GRAPH_EDIT_H

#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include "ideam_graph_node.h"
#include "../../core/graphs/ideam_graph.h"

namespace godot {

class IdeamGraphEdit : public GraphEdit {
    GDCLASS(IdeamGraphEdit, GraphEdit)

private:
    // The link to the Core Functional Graph
    ideam::core::IdeamGraph* core_graph = nullptr;

    // UI Elements
    PopupMenu* context_popup = nullptr;
    Vector2 popup_position;
    
    // Selection state for context actions
    IdeamGraphNode* context_node = nullptr;

    // Dependency Injected by Inspector Plugin
    Object* undo_redo = nullptr;

protected:
    static void _bind_methods();

    // Signal Handlers
    void _request_connect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port);
    void _request_disconnect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port);
    void _show_popup(const Vector2 &p_at);
    void _popup_select(int p_id);

    // Internal Helpers
    void _create_popup();
    virtual TypedArray<String> _get_new_node_types() const;
    virtual void _spawn_node_by_type(int p_type_id);

public:
    IdeamGraphEdit();
    virtual ~IdeamGraphEdit() override;

    void _ready() override;

    // --- Core Integration ---
    void set_core_graph(ideam::core::IdeamGraph* p_core) { core_graph = p_core; }
    ideam::core::IdeamGraph* get_core_graph() const { return core_graph; }

    void clear_all_nodes();
    void node_context_clicked(IdeamGraphNode* p_node);

    // Property Hook for Inspector Injection
    void set_undo_redo(Object* p_ur) { undo_redo = p_ur; }
    Object* get_undo_redo() const { return undo_redo; }
};

} // namespace godot

#endif // IDEAM_GRAPH_EDIT_H