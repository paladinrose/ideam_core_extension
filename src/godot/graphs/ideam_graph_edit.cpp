#include "ideam_graph_edit.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {

void IdeamGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_request_connect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_connect);
    ClassDB::bind_method(D_METHOD("_request_disconnect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_disconnect);
    ClassDB::bind_method(D_METHOD("_show_popup", "at"), &IdeamGraphEdit::_show_popup);
    ClassDB::bind_method(D_METHOD("_popup_select", "id"), &IdeamGraphEdit::_popup_select);
    ClassDB::bind_method(D_METHOD("node_context_clicked", "node"), &IdeamGraphEdit::node_context_clicked);

    // Property for UndoRedo injection
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_undo_redo", "get_undo_redo");
}

IdeamGraphEdit::IdeamGraphEdit() {
    // In a real scenario, this might be passed in or owned by a Resource
    // For now, we ensure the UI can function even without a core back-end
}

IdeamGraphEdit::~IdeamGraphEdit() {
}

void IdeamGraphEdit::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        connect("connection_request", Callable(this, "_request_connect"));
        connect("disconnection_request", Callable(this, "_request_disconnect"));
        connect("popup_request", Callable(this, "_show_popup"));
        _create_popup();
    }
}

void IdeamGraphEdit::_create_popup() {
    context_popup = memnew(PopupMenu);
    add_child(context_popup);
    context_popup->connect("index_pressed", Callable(this, "_popup_select"));
}

void IdeamGraphEdit::_show_popup(const Vector2 &p_at) {
    if (!context_popup) return;

    popup_position = get_local_mouse_position();
    context_popup->clear();

    if (context_node) {
        // Handle node-specific context menus
        TypedArray<String> options = context_node->get_context_menu_options();
        for (int i = 0; i < options.size(); ++i) {
            context_popup->add_item(options[i]);
        }
    } else {
        // Handle global "Create Node" menu
        TypedArray<String> types = _get_new_node_types();
        for (int i = 0; i < types.size(); ++i) {
            context_popup->add_item(types[i]);
        }
    }

    context_popup->set_position(get_screen_position() + popup_position);
    context_popup->popup();
}

void IdeamGraphEdit::_popup_select(int p_id) {
    if (context_node) {
        context_node->select_context_menu_option(p_id);
        context_node = nullptr;
    } else {
        _spawn_node_by_type(p_id);
    }
}

void IdeamGraphEdit::_request_connect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port) {
    // Visual Connection
    Error err = connect_node(p_from_node, p_from_port, p_to_node, p_to_port);
    
    // Core Connectivity
    if (err == OK && core_graph) {
        // Note: Real implementation would require mapping Godot Node names to Core NodeIDs
        // core_graph->connect_nodes(...);
    }
}

void IdeamGraphEdit::_request_disconnect(const String &p_from_node, int p_from_port, const String &p_to_node, int p_to_port) {
    disconnect_node(p_from_node, p_from_port, p_to_node, p_to_port);
    
    if (core_graph) {
        // core_graph->disconnect_nodes(...);
    }
}

void IdeamGraphEdit::_spawn_node_by_type(int p_type_id) {
    IdeamGraphNode *new_node = memnew(IdeamGraphNode);
    add_child(new_node);
    
    new_node->set_position_offset(popup_position + get_scroll_offset());
    
    // Core Sync
    if (core_graph) {
        ideam::core::NodeID cid = core_graph->add_node(p_type_id);
        new_node->set_core_node_id(cid);
    }
}

TypedArray<String> IdeamGraphEdit::_get_new_node_types() const {
    TypedArray<String> arr;
    arr.push_back("Ideam Graph Node");
    arr.push_back("Sub-Graph Node");
    return arr;
}

void IdeamGraphEdit::node_context_clicked(IdeamGraphNode* p_node) {
    context_node = p_node;
    _show_popup(get_local_mouse_position());
}

void IdeamGraphEdit::clear_all_nodes() {
    clear_connections();
    
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); ++i) {
        IdeamGraphNode *gn = Object::cast_to<IdeamGraphNode>(children[i]);
        if (gn) {
            gn->queue_free();
        }
    }

    if (core_graph) {
        core_graph->clear();
    }
}

} // namespace godot