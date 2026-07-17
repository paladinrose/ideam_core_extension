#include "ideam_graph_edit.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/graph_frame.hpp>

using namespace godot;

namespace ideam::godot_ext {



IdeamGraphEdit::IdeamGraphEdit() {
}

IdeamGraphEdit::~IdeamGraphEdit() {
}

void IdeamGraphEdit::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_request_connect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_connect);
    ClassDB::bind_method(D_METHOD("_request_disconnect", "from_node", "from_port", "to_node", "to_port"), &IdeamGraphEdit::_request_disconnect);
    ClassDB::bind_method(D_METHOD("_show_popup", "at", "from_empty"), &IdeamGraphEdit::_show_popup);
    ClassDB::bind_method(D_METHOD("_on_context_click", "at"), &IdeamGraphEdit::_on_context_click);
    ClassDB::bind_method(D_METHOD("_popup_select", "id"), &IdeamGraphEdit::_popup_select);
    ClassDB::bind_method(D_METHOD("node_context_clicked", "node"), &IdeamGraphEdit::node_context_clicked);
    ClassDB::bind_method(D_METHOD("_on_blueprint_changed"), &IdeamGraphEdit::_on_blueprint_changed);
    ClassDB::bind_method(D_METHOD("_on_end_node_move"), &IdeamGraphEdit::_on_end_node_move);
    
    ClassDB::bind_method(D_METHOD("_on_node_property_changed", "node_name", "property_name", "new_value"), &IdeamGraphEdit::_on_node_property_changed);
    ClassDB::bind_method(D_METHOD("_on_node_connections_requested", "node"), &IdeamGraphEdit::_on_node_connections_requested);

    ClassDB::bind_method(D_METHOD("_frame_attached", "element", "frame"), &IdeamGraphEdit::_frame_attached);
    ClassDB::bind_method(D_METHOD("_frame_detached", "element", "frame"), &IdeamGraphEdit::_frame_detached);

    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &IdeamGraphEdit::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &IdeamGraphEdit::get_blueprint);
    
}

void IdeamGraphEdit::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        connect("connection_request", Callable(this, "_request_connect"));
        connect("disconnection_request", Callable(this, "_request_disconnect"));
        
        connect("popup_request", Callable(this, "_on_context_click"));
        connect("end_node_move", Callable(this, "_on_end_node_move"));

        //connect("frame_attached", Callable(this, "_frame_attached"));
        //connect("frame_detached", Callable(this, "_frame_detached"));
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
    if (from_ign) _on_node_connections_requested(from_ign);
    if (to_ign) _on_node_connections_requested(to_ign);

}

void IdeamGraphEdit::_request_disconnect(const StringName &p_from_node, int p_from_port, const StringName &p_to_node, int p_to_port) {
    if (current_blueprint.is_null()) return;
    current_blueprint->action_remove_edge(p_from_node, p_from_port, p_to_node, p_to_port);
    
    Node* from_n = get_node_or_null(NodePath(p_from_node));
    Node* to_n = get_node_or_null(NodePath(p_to_node));
    
    if (from_n) _on_node_connections_requested(from_n);
    if (to_n) _on_node_connections_requested(to_n);
}

IdeamGraphNode* IdeamGraphEdit::_create_graph_node(const Ref<IdeamGraphNodeResource>& p_node_res) {
    // Default implementation returns nullptr. 
    // Derived graph editors must override this to handle their specific concrete types.
    return nullptr;
}

void IdeamGraphEdit::_on_context_click(const Vector2 &p_at) {
    _show_popup(p_at, false);
}

void IdeamGraphEdit::_show_popup(const Vector2 &p_at, bool p_from_empty) {
    _create_popup();
    context_popup->clear();
    context_node = nullptr; 
    popup_position = p_at;

    if (!p_from_empty) {
        
        context_popup->add_item("Create Node Group", MENU_CREATE_GROUP);
        context_popup->add_separator("Spawn Node Types");
    }
    TypedArray<String> types = _get_new_node_types();
    for (int i = 0; i < types.size(); ++i) {
        context_popup->add_item(types[i], MENU_SPAWN_NODE_START + i);
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
        Vector2 screen_mouse_pos = get_screen_position() + get_local_mouse_position();
        context_popup->set_position(screen_mouse_pos);
        context_popup->popup();
    }
}

void IdeamGraphEdit::_popup_select(int p_id) {
    if (context_node) {
        context_node->set_context_hover(false);
        context_node->select_context_menu_option(p_id);
    } else {
        if (p_id == MENU_CREATE_GROUP) {
            if (current_blueprint.is_valid()) {
                Ref<IdeamGraphGroupResource> group_res;
                group_res.instantiate();
                String unique_id = "group_" + String::num_int64(UtilityFunctions::randi() % 100000);
                group_res->set_group_name(unique_id);
                group_res->set_title("New Node Group Container");
                group_res->set_position(get_scroll_offset() + popup_position);
                group_res->set_size(Vector2(250, 200));
                
                current_blueprint->action_create_group(group_res);
            }
        } else if (p_id == MENU_REMOVE_GROUP) {
            if(current_blueprint.is_valid()) {
                // This example assumes you have logic to determine which group is under the cursor
                // You would need to implement hit detection against group frames to find the correct group_id
                StringName group_id_to_remove = "some_logic_to_find_group_under_cursor";
                current_blueprint->action_remove_group(group_id_to_remove);
            }
        } else if (p_id >= MENU_SPAWN_NODE_START) {
            _spawn_node_by_type(p_id - MENU_SPAWN_NODE_START);
        }
    }
    context_node = nullptr;
}

TypedArray<String> IdeamGraphEdit::_get_new_node_types() const { return TypedArray<String>(); }

void IdeamGraphEdit::_spawn_node_by_type(int p_type_id) {}



void IdeamGraphEdit::_on_node_property_changed(const StringName& p_node_name, const StringName& p_property_name, const Variant& p_new_value) {
    if (current_blueprint.is_null() || is_syncing_ui) return;

    // Direct O(1) Resource Property Mapping
    Node* existing_node = get_node_or_null(NodePath(p_node_name));
    IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(existing_node);

    if (ign && ign->get_node_resource().is_valid()) {
        ign->get_node_resource()->set(p_property_name, p_new_value);
    }
}

void IdeamGraphEdit::_on_node_connections_requested(Object* p_node) {
    IdeamGraphNode* ign = Object::cast_to<IdeamGraphNode>(p_node);
    if (!ign || current_blueprint.is_null()) return;

    StringName target_node_name = ign->get_blueprint_id();

    TypedArray<Dictionary> inputs;
    TypedArray<Dictionary> outputs;

    TypedArray<Dictionary> edges = current_blueprint->get_edges();
    for (int i = 0; i < edges.size(); ++i) {
        Dictionary e = edges[i];
        
        if (e["to"] == Variant(target_node_name)) {
            Dictionary conn;
            conn["name"] = String("input_port_") + String::num_int64(e["to_port"]);
            conn["type"] = Variant::OBJECT; // Emulating Godot reflection typing
            conn["usage"] = PROPERTY_USAGE_DEFAULT;
            
            // Your custom connection payload
            conn["port"] = e["to_port"];
            conn["connected_node"] = e["from"];
            conn["connected_port"] = e["from_port"];
            inputs.push_back(conn);
        }
        
        if (e["from"] == Variant(target_node_name)) {
            Dictionary conn;
            conn["name"] = String("output_port_") + String::num_int64(e["from_port"]);
            conn["type"] = Variant::OBJECT; 
            conn["usage"] = PROPERTY_USAGE_DEFAULT;
            
            // Your custom connection payload
            conn["port"] = e["from_port"];
            conn["connected_node"] = e["to"];
            conn["connected_port"] = e["to_port"];
            outputs.push_back(conn);
        }
    }

    Dictionary payload;
    payload["inputs"] = inputs;
    payload["outputs"] = outputs;

    ign->receive_connection_info(payload);
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
            ign->update_from_resource(n_res);
            
        } else {
            //This should be somehow harnessing _spawn_node_by_type, so start here, later.
            ign = _create_graph_node(n_res);
            
            if (ign) {
                add_child(ign);
                ign->set_theme(get_theme());
                ign->initialize(n_res);
                
            }
        }
        
    }

    TypedArray<Ref<IdeamGraphGroupResource>> blueprint_groups = current_blueprint->get_groups();
    for (int i = 0; i < blueprint_groups.size(); ++i) {
        Ref<IdeamGraphGroupResource> g_res = blueprint_groups[i];
        if (g_res.is_null() || g_res->get_group_name().is_empty()) continue;

        StringName group_id = g_res->get_group_name();
        Node* existing_frame = get_node_or_null(NodePath(group_id));
        GraphFrame* gf = Object::cast_to<GraphFrame>(existing_frame);

        if (!gf) {
            gf = memnew(GraphFrame);
            gf->set_name(group_id);
            add_child(gf);
        }

        gf->set_title(g_res->get_title());
        gf->set_position_offset(g_res->get_position());
        gf->set_size(g_res->get_size());

        // Map child dependencies to the frame layout canvas
        TypedArray<StringName> associated_nodes = g_res->get_nodes();
        for(int j = 0; j < associated_nodes.size(); ++j) {
            Node* target_node = get_node_or_null(NodePath(associated_nodes[j]));
            GraphElement* ge = Object::cast_to<GraphElement>(target_node);
            if (ge && ge->get_parent() == this) {
                attach_graph_element_to_frame(ge->get_name(), group_id);
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

void IdeamGraphEdit::_frame_attached(const StringName& p_element, const StringName& p_frame) {
    if (current_blueprint.is_null() || is_syncing_ui) return;
    current_blueprint->action_attach_to_group(p_frame, p_element);
}

void IdeamGraphEdit::_frame_detached(const StringName& p_element, const StringName& p_frame) {
    if (current_blueprint.is_null() || is_syncing_ui) return;
    current_blueprint->action_detach_from_group(p_frame, p_element);
}

} // namespace ideam::godot_ext