#include "ideam_graph_edit.h"
// Include the actual resource definition (adjust path as needed for your folder structure)
#include "ideam_graph_resource.h" 

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

void IdeamGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_request_connect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_connect);
    ClassDB::bind_method(D_METHOD("_request_disconnect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_disconnect);
    ClassDB::bind_method(D_METHOD("_show_popup", "at"), &IdeamGraphEdit::_show_popup);
    ClassDB::bind_method(D_METHOD("_popup_select", "id"), &IdeamGraphEdit::_popup_select);
    ClassDB::bind_method(D_METHOD("node_context_clicked", "node"), &IdeamGraphEdit::node_context_clicked);
    ClassDB::bind_method(D_METHOD("_on_blueprint_changed"), &IdeamGraphEdit::_on_blueprint_changed);
    ClassDB::bind_method(D_METHOD("_on_end_node_move"), &IdeamGraphEdit::_on_end_node_move);

    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &IdeamGraphEdit::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &IdeamGraphEdit::get_blueprint);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blueprint", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphResource"), "set_blueprint", "get_blueprint");
}

IdeamGraphEdit::IdeamGraphEdit() {
    // Enable connection validation
    set_right_disconnects(true);
}

IdeamGraphEdit::~IdeamGraphEdit() {
}

void IdeamGraphEdit::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        // UI Setup
    }

    _create_popup();

    connect("connection_request", Callable(this, "_request_connect"));
    connect("disconnection_request", Callable(this, "_request_disconnect"));
    connect("popup_request", Callable(this, "_show_popup"));
    connect("end_node_move", Callable(this, "_on_end_node_move"));
}

void IdeamGraphEdit::set_blueprint(const Ref<ideam::godot_ext::IdeamGraphResource>& p_blueprint) {
    // Disconnect old blueprint
    if (current_blueprint.is_valid() && current_blueprint->is_connected("changed", Callable(this, "_on_blueprint_changed"))) {
        current_blueprint->disconnect("changed", Callable(this, "_on_blueprint_changed"));
    }

    current_blueprint = p_blueprint;

    // Connect new blueprint
    if (current_blueprint.is_valid()) {
        current_blueprint->connect("changed", Callable(this, "_on_blueprint_changed"));
        _on_blueprint_changed(); // Force an immediate UI sync
    } else {
        clear_connections();
        // Clear child nodes
        TypedArray<Node> children = get_children();
        for (int i = 0; i < children.size(); ++i) {
            Node* child = Object::cast_to<Node>(children[i]);
            if (Object::cast_to<IdeamGraphNode>(child)) {
                child->queue_free();
            }
        }
    }
}

// ==========================================
// REACTIVE SYNC (Model -> View)
// ==========================================

void IdeamGraphEdit::_on_blueprint_changed() {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    is_syncing_ui = true;

    // 1. Sync Nodes
    TypedArray<Dictionary> bp_nodes = current_blueprint->get_nodes();
    std::vector<StringName> valid_node_names;
    
    for (int i = 0; i < bp_nodes.size(); ++i) {
        Dictionary n_data = bp_nodes[i];
        if (!n_data.has("name")) continue;
        
        StringName n_name = n_data["name"];
        valid_node_names.push_back(n_name);

        Node* existing_node = get_node_or_null(NodePath(n_name));
        IdeamGraphNode* g_node = Object::cast_to<IdeamGraphNode>(existing_node);

        if (g_node) {
            // Update existing node
            g_node->initialize(n_data); 
        } else {
            // Create missing node
            g_node = memnew(IdeamGraphNode);
            add_child(g_node);
            g_node->initialize(n_data); // Automatically sets name, position, and builds UI
        }
    }

    // 2. Cull Deleted Nodes
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); ++i) {
        IdeamGraphNode* g_node = Object::cast_to<IdeamGraphNode>(children[i]);
        if (g_node) {
            StringName g_name = g_node->get_name();
            bool found = false;
            for (const StringName& valid_name : valid_node_names) {
                if (g_name == valid_name) { found = true; break; }
            }
            if (!found) {
                g_node->queue_free();
            }
        }
    }

    // 3. Sync Edges
    clear_connections();
    TypedArray<Dictionary> bp_edges = current_blueprint->get_edges();
    
    for (int i = 0; i < bp_edges.size(); ++i) {
        Dictionary e_data = bp_edges[i];
        if (e_data.has("from") && e_data.has("to")) {
            StringName from = e_data["from"];
            StringName to = e_data["to"];
            int from_port = e_data.has("from_port") ? static_cast<int>(e_data["from_port"]) : 0;
            int to_port = e_data.has("to_port") ? static_cast<int>(e_data["to_port"]) : 0;
            
            connect_node(from, from_port, to, to_port);
        }
    }

    is_syncing_ui = false;
}

// ==========================================
// INPUT ROUTING (View -> Model)
// ==========================================

void IdeamGraphEdit::_request_connect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;

    Dictionary edge;
    edge["from"] = p_from_node;
    edge["from_port"] = p_from_port;
    edge["to"] = p_to_node;
    edge["to_port"] = p_to_port;

    // Send the action to the blueprint. It will handle Undo/Redo and emit 'changed' to draw the line.
    current_blueprint->action_add_edge(edge);
}

void IdeamGraphEdit::_request_disconnect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;

    current_blueprint->action_remove_edge(p_from_node, p_from_port, p_to_node, p_to_port);
}

void IdeamGraphEdit::_on_end_node_move() {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    // When the user finishes dragging, we snapshot the new positions of all nodes to the blueprint.
    // In a full implementation, you would route this through the Undo/Redo manager on the resource.
    TypedArray<Dictionary> new_nodes = current_blueprint->get_nodes().duplicate(true);
    
    for (int i = 0; i < new_nodes.size(); ++i) {
        Dictionary n_data = new_nodes[i];
        if (n_data.has("name")) {
            Node* ui_node = get_node_or_null(NodePath(n_data["name"]));
            IdeamGraphNode* g_node = Object::cast_to<IdeamGraphNode>(ui_node);
            if (g_node) {
                n_data["position"] = g_node->get_position_offset();
                new_nodes[i] = n_data;
            }
        }
    }
    
    current_blueprint->set_nodes(new_nodes);
}

void IdeamGraphEdit::_spawn_node_by_type(int p_type_id) {
    if (current_blueprint.is_null()) return;

    // Generate a unique StringName for the node
    StringName unique_name = String("Node_") + String::num_int64(UtilityFunctions::randi());

    Dictionary new_node;
    new_node["name"] = unique_name;
    new_node["type_id"] = p_type_id;
    new_node["position"] = popup_position + get_scroll_offset();
    
    // Route creation through the blueprint
    current_blueprint->action_add_node(new_node);
}

// ==========================================
// POPUP / CONTEXT MENUS
// ==========================================

void IdeamGraphEdit::_create_popup() {
    if (context_popup) return;
    
    context_popup = memnew(PopupMenu);
    add_child(context_popup);
    context_popup->connect("id_pressed", Callable(this, "_popup_select"));
}

void IdeamGraphEdit::_show_popup(const Vector2 &p_at) {
    if (!context_popup) return;
    
    context_popup->clear();
    context_node = nullptr; 

    TypedArray<String> types = _get_new_node_types();
    for (int i = 0; i < types.size(); ++i) {
        context_popup->add_item(types[i], i);
    }
    
    popup_position = p_at;
    context_popup->set_position(Vector2i(get_global_mouse_position().x, get_global_mouse_position().y));
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

TypedArray<String> IdeamGraphEdit::_get_new_node_types() const {
    TypedArray<String> arr;
    arr.push_back("Ideam Graph Node");
    arr.push_back("Sub-Graph Node");
    return arr;
}

void IdeamGraphEdit::node_context_clicked(IdeamGraphNode* p_node) {
    if (!context_popup || !p_node) return;

    context_popup->clear();
    context_node = p_node;

    TypedArray<String> options = p_node->get_context_menu_options();
    for (int i = 0; i < options.size(); ++i) {
        context_popup->add_item(options[i], i);
    }

    if (context_popup->get_item_count() > 0) {
        context_popup->set_position(Vector2i(get_global_mouse_position().x, get_global_mouse_position().y));
        context_popup->popup();
    }
}

} // namespace godot