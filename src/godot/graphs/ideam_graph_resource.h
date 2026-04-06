#ifndef IDEAM_GODOT_GRAPH_RESOURCE_H
#define IDEAM_GODOT_GRAPH_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <memory>
#include <unordered_map>

#include "../../core/graphs/ideam_graph_dod.h"

namespace ideam::godot_ext {

class IdeamGraphResource : public godot::Resource {
    GDCLASS(IdeamGraphResource, godot::Resource)

private:
    godot::TypedArray<godot::Dictionary> nodes;
    godot::TypedArray<godot::Dictionary> edges;
    bool is_volatile_at_runtime = false;

    // Injected by the Inspector or GraphEdit UI
    godot::Object* undo_redo = nullptr;

protected:
    static void _bind_methods();

public:
    IdeamGraphResource() = default;
    ~IdeamGraphResource() = default;

    // --- State Properties ---
    void set_nodes(const godot::TypedArray<godot::Dictionary>& p_nodes) { nodes = p_nodes; emit_changed(); }
    godot::TypedArray<godot::Dictionary> get_nodes() const { return nodes; }

    void set_edges(const godot::TypedArray<godot::Dictionary>& p_edges) { edges = p_edges; emit_changed(); }
    godot::TypedArray<godot::Dictionary> get_edges() const { return edges; }

    void set_is_volatile(bool p_volatile) { is_volatile_at_runtime = p_volatile; emit_changed(); }
    bool get_is_volatile() const { return is_volatile_at_runtime; }

    void set_undo_redo(godot::Object* p_undo_redo) { undo_redo = p_undo_redo; }
    godot::Object* get_undo_redo() const { return undo_redo; }

    // --- Tier 1: Action Routers (Called by UI) ---
    void action_add_node(const godot::Dictionary& p_node_data);
    void action_remove_node(const godot::StringName& p_node_name);
    void action_add_edge(const godot::Dictionary& p_edge_data);
    void action_remove_edge(const godot::StringName& p_from, int p_from_port, const godot::StringName& p_to, int p_to_port);

    // --- Tier 2: Direct Execution (The "Do" / "Undo" Targets) ---
    void _do_add_node(const godot::Dictionary& p_node_data);
    void _undo_add_node(const godot::StringName& p_node_name);
    
    void _do_add_edge(const godot::Dictionary& p_edge_data);
    void _undo_add_edge(const godot::StringName& p_from, int p_from_port, const godot::StringName& p_to, int p_to_port);

    // --- The DOD Compiler ---
    std::shared_ptr<core::IdeamGraphDOD> compile_to_dod(
        core::MemoryManagerDOD* p_manager, 
        std::unordered_map<godot::String, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext

#endif // IDEAM_GODOT_GRAPH_RESOURCE_H