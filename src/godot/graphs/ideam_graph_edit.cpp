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
        // Connect to Godot's built-in GraphEdit signals
        connect("connection_request", Callable(this, "_request_connect"));
        connect("disconnection_request", Callable(this, "_request_disconnect"));
        connect("popup_request", Callable(this, "_show_popup"));
        connect("end_node_move", Callable(this, "_on_end_node_move"));
    }
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

// ==========================================
// TIER 1: STRICT ACCESS CONTROL & ROUTING
// ==========================================

void IdeamGraphEdit::_request_connect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;

    // PRE-FLIGHT CHECK: Enforce Port Immutability
    // We poll the UI nodes to check if they are currently locked by the DOD state
    Node* from_n = get_node_or_null(NodePath(p_from_node));
    Node* to_n = get_node_or_null(NodePath(p_to_node));
    
    IdeamGraphNode* from_ign = Object::cast_to<IdeamGraphNode>(from_n);
    IdeamGraphNode* to_ign = Object::cast_to<IdeamGraphNode>(to_n);

    // If either node is fully locked, reject the structural mutation immediately.
    // In a fully fleshed out port validation, we would also check exact port compatibility here.
    if ((from_ign && from_ign->get_properties().has("locked") && from_ign->get_properties()["locked"]) || 
        (to_ign && to_ign->get_properties().has("locked") && to_ign->get_properties()["locked"])) {
        
        // Operation Rejected - We do not push to the blueprint
        // Could temporarily set port_error_color here for visual feedback
        return;
    }

    // Validation passed, defer mutation to the DOD Resource
    Dictionary edge;
    edge["from"] = p_from_node;
    edge["from_port"] = p_from_port;
    edge["to"] = p_to_node;
    edge["to_port"] = p_to_port;
    
    current_blueprint->action_add_edge(edge);
}

void IdeamGraphEdit::_request_disconnect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;

    Node* from_n = get_node_or_null(NodePath(p_from_node));
    Node* to_n = get_node_or_null(NodePath(p_to_node));
    
    IdeamGraphNode* from_ign = Object::cast_to<IdeamGraphNode>(from_n);
    IdeamGraphNode* to_ign = Object::cast_to<IdeamGraphNode>(to_n);

    if ((from_ign && from_ign->get_properties().has("locked") && from_ign->get_properties()["locked"]) || 
        (to_ign && to_ign->get_properties().has("locked") && to_ign->get_properties()["locked"])) {
        return; // Reject operation
    }

    current_blueprint->action_remove_edge(p_from_node, p_from_port, p_to_node, p_to_port);
}

// ==========================================
// CONTEXT MENUS & DYNAMIC STYLING
// ==========================================

void IdeamGraphEdit::_create_popup() {
    if (context_popup) return;

    context_popup = memnew(PopupMenu);
    add_child(context_popup);

    // Dynamically inject custom UI styles mapped from the global Theme cascade.
    // Because we use get_theme_stylebox, if the user edits the .tres file, 
    // the popup updates automatically without requiring a recompile.
    Ref<StyleBox> panel_style = get_theme_stylebox("popup_menu_panel");
    if (panel_style.is_valid()) {
        context_popup->add_theme_stylebox_override("panel", panel_style);
    }
    
    Ref<StyleBox> hover_style = get_theme_stylebox("popup_menu_hover");
    if (hover_style.is_valid()) {
        context_popup->add_theme_stylebox_override("hover", hover_style);
    }

    context_popup->connect("id_pressed", Callable(this, "_popup_select"));
}

void IdeamGraphEdit::_show_popup(const Vector2 &p_at) {
    _create_popup();
    context_popup->clear();
    context_node = nullptr; // Clear active node; this is a canvas click
    popup_position = p_at;

    // Populate Canvas-level options (e.g., spawn nodes)
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
    
    // Highlight the target node visually using the DOD interaction state
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
        // Clear the hover state highlight
        context_node->set_context_hover(false);
        // Delegate action directly to the node (e.g., "Delete Node")
        context_node->select_context_menu_option(p_id);
    } else {
        // Canvas action (e.g., spawn new DOD task)
        _spawn_node_by_type(p_id);
    }
    context_node = nullptr;
}

TypedArray<String> IdeamGraphEdit::_get_new_node_types() const {
    // Intended to be overridden by derived classes (TaskGraphEdit, MemoryGraphEdit)
    return TypedArray<String>();
}

void IdeamGraphEdit::_spawn_node_by_type(int p_type_id) {
    // Intended to be overridden
}

// ==========================================
// NODE SIGNAL ROUTING (View -> Model)
// ==========================================

void IdeamGraphEdit::_on_node_delete_request(const StringName& p_node_name) {
    if (current_blueprint.is_null()) return;
    current_blueprint->action_remove_node(p_node_name);
}

void IdeamGraphEdit::_on_node_property_changed(const StringName& p_node_name, const StringName& p_property_name, const Variant& p_new_value) {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    // Fetch, update, and push the property modification to the DOD Resource.
    TypedArray<Dictionary> nodes = current_blueprint->get_nodes().duplicate(true);
    
    for (int i = 0; i < nodes.size(); ++i) {
        Dictionary n_data = nodes[i];
        if (n_data.has("name") && static_cast<StringName>(n_data["name"]) == p_node_name) {
            
            Dictionary props;
            if (n_data.has("properties")) {
                props = n_data["properties"];
            }
            
            props[p_property_name] = p_new_value;
            n_data["properties"] = props;
            nodes[i] = n_data;
            break;
        }
    }
    
    current_blueprint->set_nodes(nodes);
}

void IdeamGraphEdit::_on_end_node_move() {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    TypedArray<Dictionary> dod_nodes = current_blueprint->get_nodes().duplicate(true);
    
    // Sync the final visual positions of all nodes back to the DOD metadata
    for (int i = 0; i < get_child_count(); ++i) {
        IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(get_child(i));
        if (!ign) continue;

        for (int j = 0; j < dod_nodes.size(); ++j) {
            Dictionary n_data = dod_nodes[j];
            if (n_data.has("name") && static_cast<StringName>(n_data["name"]) == ign->get_blueprint_id()) {
                n_data["position"] = ign->get_position_offset();
                dod_nodes[j] = n_data;
                break;
            }
        }
    }

    current_blueprint->set_nodes(dod_nodes);
}

// ==========================================
// REACTIVE UI SYNC (Model -> View)
// ==========================================

void IdeamGraphEdit::_on_blueprint_changed() {
    if (is_syncing_ui || current_blueprint.is_null()) return;
    
    is_syncing_ui = true;

    // 1. Clear current UI lines
    clear_connections();

    // 2. Diff and sync nodes
    TypedArray<Dictionary> dod_nodes = current_blueprint->get_nodes();
    std::vector<StringName> current_dod_ids;

    for (int i = 0; i < dod_nodes.size(); ++i) {
        Dictionary n_data = dod_nodes[i];
        if (!n_data.has("name")) continue;
        
        StringName dod_id = n_data["name"];
        current_dod_ids.push_back(dod_id);

        Node* existing_node = get_node_or_null(NodePath(dod_id));
        IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(existing_node);

        if (ign) {
            // Update existing
            ign->initialize(n_data);
            if (n_data.has("position")) {
                ign->set_position_offset(n_data["position"]);
            }
        } else {
            // In a fully implemented system, we would instantiate the correct 
            // derived class based on type_id here. For now, we assume derived classes 
            // will override _on_blueprint_changed or _spawn_node_by_type.
        }
    }

    // 3. Rebuild edges
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