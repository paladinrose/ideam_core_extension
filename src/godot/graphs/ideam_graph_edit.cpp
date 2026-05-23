#include "ideam_graph_edit.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/control.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamGraphEdit::IdeamGraphEdit() {
}

IdeamGraphEdit::~IdeamGraphEdit() {
}

void IdeamGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_request_connect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_connect);
    ClassDB::bind_method(D_METHOD("_request_disconnect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_disconnect);
    ClassDB::bind_method(D_METHOD("_show_popup", "at"), &IdeamGraphEdit::_show_popup);
    ClassDB::bind_method(D_METHOD("_popup_select", "id"), &IdeamGraphEdit::_popup_select);
    ClassDB::bind_method(D_METHOD("node_context_clicked", "node"), &IdeamGraphEdit::node_context_clicked);
    ClassDB::bind_method(D_METHOD("_on_blueprint_changed"), &IdeamGraphEdit::_on_blueprint_changed);
    ClassDB::bind_method(D_METHOD("_on_end_node_move"), &IdeamGraphEdit::_on_end_node_move);
    
    ClassDB::bind_method(D_METHOD("_on_node_property_changed", "node_name", "property_name", "new_value"), &IdeamGraphEdit::_on_node_property_changed);
    ClassDB::bind_method(D_METHOD("_on_node_delete_request", "node_name"), &IdeamGraphEdit::_on_node_delete_request);

    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &IdeamGraphEdit::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &IdeamGraphEdit::get_blueprint);
}

void IdeamGraphEdit::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        connect("connection_request", Callable(this, "_request_connect"));
        connect("disconnection_request", Callable(this, "_request_disconnect"));
        connect("popup_request", Callable(this, "_show_popup"));
        connect("end_node_move", Callable(this, "_on_end_node_move"));
    }
}

void IdeamGraphEdit::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme_properties();
        } break;
    }
}

void IdeamGraphEdit::_create_popup() {
    if (context_popup) return;

    context_popup = memnew(PopupMenu);
    add_child(context_popup);

    // Call the styling forwarder immediately upon creation
    _update_theme_properties();

    context_popup->connect("id_pressed", Callable(this, "_popup_select"));
}

void IdeamGraphEdit::_update_theme_properties() {
    
}

void IdeamGraphEdit::set_blueprint(const Ref<IdeamGraphResource>& p_blueprint) {
    if (current_blueprint == p_blueprint) return;

    if (current_blueprint.is_valid()) {
        current_blueprint->disconnect("changed", Callable(this, "_on_blueprint_changed"));
    }

    current_blueprint = p_blueprint;

    if (current_blueprint.is_valid()) {
        current_blueprint->connect("changed", Callable(this, "_on_blueprint_changed"));
    }

    _on_blueprint_changed();
}

void IdeamGraphEdit::_request_connect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;

    Node* from_n = get_node_or_null(NodePath(p_from_node));
    Node* to_n = get_node_or_null(NodePath(p_to_node));
    
    IdeamGraphNode* from_ign = Object::cast_to<IdeamGraphNode>(from_n);
    IdeamGraphNode* to_ign = Object::cast_to<IdeamGraphNode>(to_n);

    if (from_ign && from_ign->get_locked()) return;
    if (to_ign && to_ign->get_locked()) return;
    
    Dictionary edge;
    edge["from"] = p_from_node;
    edge["from_port"] = p_from_port;
    edge["to"] = p_to_node;
    edge["to_port"] = p_to_port;
    
    current_blueprint->action_add_edge(edge);
}

void IdeamGraphEdit::_request_disconnect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;
    current_blueprint->action_remove_edge(p_from_node, p_from_port, p_to_node, p_to_port);
}

IdeamGraphNode* IdeamGraphEdit::_create_graph_node(const Ref<IdeamGraphNodeResource>& p_node_res) {
    // Default implementation returns nullptr. 
    // Derived graph editors must override this to handle their specific concrete types.
    return nullptr;
}

void IdeamGraphEdit::_show_popup(const Vector2 &p_at) {
    _create_popup();
    context_popup->clear();
    context_node = nullptr; 
    popup_position = p_at;

    TypedArray<String> types = _get_new_node_types();
    for (int i = 0; i < types.size(); ++i) {
        context_popup->add_item(types[i], i);
    }

    if (context_popup->get_item_count() > 0) {
        context_popup->set_position(get_screen_position() + p_at);
        context_popup->popup();
    }
}

void IdeamGraphEdit::node_context_clicked(Object* p_node) {
    IdeamGraphNode* node = Object::cast_to<IdeamGraphNode>(p_node);
    if (!node) return;

    _create_popup();
    context_popup->clear();
    context_node = node;
    context_node->set_context_hover(true);

    TypedArray<String> options = node->get_context_menu_options();
    for (int i = 0; i < options.size(); ++i) {
        context_popup->add_item(options[i], i);
    }

    if (context_popup->get_item_count() > 0) {
        context_popup->set_position(get_viewport()->get_mouse_position());
        context_popup->popup();
    }
}

void IdeamGraphEdit::_popup_select(int p_id) {
    if (context_node) {
        context_node->set_context_hover(false);
        context_node->select_context_menu_option(p_id);
    } else {
        _spawn_node_by_type(p_id);
    }
    context_node = nullptr;
}

TypedArray<String> IdeamGraphEdit::_get_new_node_types() const { return TypedArray<String>(); }

void IdeamGraphEdit::_spawn_node_by_type(int p_type_id) {}

void IdeamGraphEdit::_on_node_delete_request(const StringName& p_node_name) {
    if (current_blueprint.is_null()) return;
    current_blueprint->action_remove_node(p_node_name);
}

void IdeamGraphEdit::_on_node_property_changed(const StringName& p_node_name, const StringName& p_property_name, const Variant& p_new_value) {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    // Direct O(1) Resource Property Mapping
    Node* existing_node = get_node_or_null(NodePath(p_node_name));
    IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(existing_node);

    if (ign && ign->get_node_resource().is_valid()) {
        ign->get_node_resource()->set(p_property_name, p_new_value);
    }
}

void IdeamGraphEdit::_on_end_node_move() {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    is_syncing_ui = true;

    // Fast O(1) loop updating Authoring Resources without heap array duplication
    for (int i = 0; i < get_child_count(); ++i) {
        IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(get_child(i));
        if (!ign) continue;

        if (ign->get_node_resource().is_valid()) {
            ign->get_node_resource()->set_position_offset(ign->get_position_offset());
        }
    }

    is_syncing_ui = false;
}

void IdeamGraphEdit::_on_blueprint_changed() {
    if (is_syncing_ui || current_blueprint.is_null()) return;
    
    is_syncing_ui = true;
    clear_connections();

    // Rebuild Nodes directly from the Typed Resource Array
    TypedArray<Ref<IdeamGraphNodeResource>> dod_nodes = current_blueprint->get_nodes();
    std::vector<StringName> current_dod_ids;

    for (int i = 0; i < dod_nodes.size(); ++i) {
        Ref<IdeamGraphNodeResource> n_res = dod_nodes[i];
        if (n_res.is_null() || n_res->get_node_name().is_empty()) continue;
        
        StringName dod_id = n_res->get_node_name();
        current_dod_ids.push_back(dod_id);

        Node* existing_node = get_node_or_null(NodePath(dod_id));
        IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(existing_node);

        if (ign) {
            ign->initialize(n_res);
        } else {
            ign = _create_graph_node(n_res);
            
            if (ign) {
                // Assuming _create_graph_node configures the node name correctly
                add_child(ign);
                ign->initialize(n_res);
            }
        }
    }

    // Rebuild Edges
    TypedArray<Dictionary> dod_edges = current_blueprint->get_edges();
    for (int i = 0; i < dod_edges.size(); ++i) {
        Dictionary e = dod_edges[i];
        if (e.has("from") && e.has("from_port") && e.has("to") && e.has("to_port")) {
            connect_node(e["from"], e["from_port"], e["to"], e["to_port"]);
        }
    }

    is_syncing_ui = false;
}

} // namespace ideam::godot_ext