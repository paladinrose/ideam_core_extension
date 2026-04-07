#include "ideam_graph_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void IdeamGraphResource::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_nodes", "nodes"), &IdeamGraphResource::set_nodes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_nodes"), &IdeamGraphResource::get_nodes);
    godot::ClassDB::bind_method(godot::D_METHOD("set_edges", "edges"), &IdeamGraphResource::set_edges);
    godot::ClassDB::bind_method(godot::D_METHOD("get_edges"), &IdeamGraphResource::get_edges);
    
    // Bind the execution methods so Godot's UndoRedo can call them by string name
    godot::ClassDB::bind_method(godot::D_METHOD("_do_add_node", "node_data"), &IdeamGraphResource::_do_add_node);
    godot::ClassDB::bind_method(godot::D_METHOD("_undo_add_node", "node_name"), &IdeamGraphResource::_undo_add_node);
    godot::ClassDB::bind_method(godot::D_METHOD("_do_add_edge", "edge_data"), &IdeamGraphResource::_do_add_edge);
    godot::ClassDB::bind_method(godot::D_METHOD("_undo_add_edge", "from", "from_port", "to", "to_port"), &IdeamGraphResource::_undo_add_edge);

    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "nodes"), "set_nodes", "get_nodes");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "edges"), "set_edges", "get_edges");
}

// ==========================================
// TIER 2: DIRECT EXECUTION (Guaranteed Sync)
// ==========================================

void IdeamGraphResource::_do_add_node(const godot::Dictionary& p_node_data) {
    nodes.append(p_node_data);
    emit_changed(); // Forces Godot Editor to recognize the change
}

void IdeamGraphResource::_undo_add_node(const godot::StringName& p_node_name) {
    for (int i = 0; i < nodes.size(); ++i) {
        godot::Dictionary dict = nodes[i];
        if (dict.has("name") && dict["name"] == godot::Variant(p_node_name)) {
            nodes.remove_at(i);
            emit_changed();
            return;
        }
    }
}

void IdeamGraphResource::_do_add_edge(const godot::Dictionary& p_edge_data) {
    edges.append(p_edge_data);
    emit_changed();
}

void IdeamGraphResource::_undo_add_edge(const godot::StringName& p_from, int p_from_port, const godot::StringName& p_to, int p_to_port) {
    for (int i = 0; i < edges.size(); ++i) {
        godot::Dictionary dict = edges[i];
        if (dict.has("from") && dict["from"] == godot::Variant(p_from) &&
            dict.has("from_port") && static_cast<int>(dict["from_port"]) == p_from_port &&
            dict.has("to") && dict["to"] == godot::Variant(p_to) &&
            dict.has("to_port") && static_cast<int>(dict["to_port"]) == p_to_port) {
            edges.remove_at(i);
            emit_changed();
            return;
        }
    }
}

// ==========================================
// TIER 1: ACTION ROUTING 
// ==========================================

void IdeamGraphResource::action_add_node(const godot::Dictionary& p_node_data) {
    if (!p_node_data.has("name")) return;
    godot::StringName n_name = p_node_data["name"];

    if (!undo_redo) {
        _do_add_node(p_node_data);
        return;
    }

    // Duck-typing avoids including Editor headers in the runtime build
    if (undo_redo->is_class("EditorUndoRedoManager")) {
        undo_redo->call("create_action", "Add Graph Node");
        undo_redo->call("add_do_method", this, "_do_add_node", p_node_data);
        undo_redo->call("add_undo_method", this, "_undo_add_node", n_name);
        undo_redo->call("commit_action");
    } 
    else if (undo_redo->is_class("UndoRedo")) {
        undo_redo->call("create_action", "Add Graph Node");
        undo_redo->call("add_do_method", this, "_do_add_node", p_node_data);
        undo_redo->call("add_undo_method", this, "_undo_add_node", n_name);
        undo_redo->call("commit_action");
    } 
    else {
        _do_add_node(p_node_data);
    }
}

void IdeamGraphResource::action_add_edge(const godot::Dictionary& p_edge_data) {
    if (!p_edge_data.has("from") || !p_edge_data.has("to")) return;
    
    godot::StringName from = p_edge_data["from"];
    int from_port = p_edge_data.has("from_port") ? static_cast<int>(p_edge_data["from_port"]) : 0;
    godot::StringName to = p_edge_data["to"];
    int to_port = p_edge_data.has("to_port") ? static_cast<int>(p_edge_data["to_port"]) : 0;

    if (!undo_redo) {
        _do_add_edge(p_edge_data);
        return;
    }

    if (undo_redo->is_class("EditorUndoRedoManager") || undo_redo->is_class("UndoRedo")) {
        undo_redo->call("create_action", "Connect Graph Nodes");
        undo_redo->call("add_do_method", this, "_do_add_edge", p_edge_data);
        undo_redo->call("add_undo_method", this, "_undo_add_edge", from, from_port, to, to_port);
        undo_redo->call("commit_action");
    } else {
        _do_add_edge(p_edge_data);
    }
}

// ==========================================
// THE DOD COMPILER
// ==========================================

std::shared_ptr<core::IdeamGraphDOD> IdeamGraphResource::compile_to_dod(
    core::MemoryManagerDOD* p_manager, 
    godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const 
{
    auto dod_graph = std::make_shared<core::IdeamGraphDOD>(p_manager);
    
    // 1. Single Allocation: Prevent vector reallocation thrashing
    dod_graph->reserve(nodes.size(), edges.size());

    r_ui_to_dod_map.clear();

    // 2. Compile Nodes
    for (int i = 0; i < nodes.size(); ++i) {
        godot::Dictionary n = nodes[i];
        if (!n.has("name") || !n.has("type_id")) continue;

        godot::StringName ui_name = n["name"];
        uint32_t type_id = static_cast<uint32_t>(n["type_id"]);

        // Push to DOD and cache the physical integer ID
        core::NodeID core_id = dod_graph->add_node(type_id);
        r_ui_to_dod_map[ui_name] = core_id;
    }

    // 3. Compile Edges
    for (int i = 0; i < edges.size(); ++i) {
        godot::Dictionary e = edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        godot::StringName from_name = e["from"];
        godot::StringName to_name = e["to"];

        // Look up the physical IDs using Godot's HashMap logic
        if (r_ui_to_dod_map.has(from_name) && r_ui_to_dod_map.has(to_name)) {
            core::NodeID from_id = r_ui_to_dod_map[from_name];
            core::NodeID to_id = r_ui_to_dod_map[to_name];
            
            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(e["from_port"]) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(e["to_port"]) : 0;

            dod_graph->connect_nodes(from_id, from_port, to_id, to_port);
        } else {
            godot::UtilityFunctions::printerr("IdeamGraphResource Compiler: Invalid edge connection. Nodes not found in UI map.");
        }
    }

    // 4. Lock the topology. Purges holes and prepares it for execution phase.
    dod_graph->defragment();

    return dod_graph;
}

} // namespace ideam::godot_ext