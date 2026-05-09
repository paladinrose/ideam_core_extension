#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <memory>

#include "../../core/graphs/ideam_graph_dod.h"
#include "../memory/memory_manager_resource.h" // Needed for the dependency handshake
#include "ideam_graph_node_resource.h" // Needed for typed array node definitions

namespace ideam::godot_ext {

class IdeamGraphResource : public godot::Resource {
    GDCLASS(IdeamGraphResource, godot::Resource)

private:
    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> nodes;
    godot::TypedArray<godot::Dictionary> edges;
    
    // --- DOD Structural Parameters ---
    godot::Ref<MemoryManagerResource> memory_manager;
    bool is_volatile_at_runtime = false;
    int volatile_node_capacity = 1024;
    int volatile_edge_capacity = 2048;

    // Injected by the Inspector or GraphEdit UI
    godot::Object* undo_redo = nullptr;

protected:
    static void _bind_methods();

    // The Virtual Pipeline: Child classes (MemoryGraphResource, TaskGraphResource) 
    // will override this, call Super::_append_managed_profiles(r_profiles), 
    // and then push their own specific utility profiles.
    virtual void _append_managed_profiles(godot::TypedArray<ManagedBufferProfile>& r_profiles) const;

public:
    IdeamGraphResource() = default;
    virtual ~IdeamGraphResource() = default; // Ensure virtual destructor for inheritance

    // --- State Properties ---
    void set_memory_manager(const godot::Ref<MemoryManagerResource>& p_manager);
    godot::Ref<MemoryManagerResource> get_memory_manager() const { return memory_manager; }

    void set_nodes(const godot::TypedArray<godot::Ref<IdeamGraphNodeResource>>& p_nodes);
    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> get_nodes() const { return nodes; }

    void set_edges(const godot::TypedArray<godot::Dictionary>& p_edges);
    godot::TypedArray<godot::Dictionary> get_edges() const { return edges; }

    void set_is_volatile(bool p_volatile);
    bool get_is_volatile() const { return is_volatile_at_runtime; }

    void set_volatile_node_capacity(int p_cap);
    int get_volatile_node_capacity() const { return volatile_node_capacity; }

    void set_volatile_edge_capacity(int p_cap);
    int get_volatile_edge_capacity() const { return volatile_edge_capacity; }

    void set_undo_redo(godot::Object* p_undo_redo) { undo_redo = p_undo_redo; }
    godot::Object* get_undo_redo() const { return undo_redo; }

    // --- Handshake Orchestration ---
    // Computes all profiles and registers them with the MemoryManagerResource
    void update_managed_profiles();

    // --- Tier 1: Action Routers (Called by UI) ---
    void action_add_node(const godot::Ref<IdeamGraphNodeResource>& p_node);
    void action_remove_node(const godot::StringName& p_node_name);
    void action_add_edge(const godot::Dictionary& p_edge_data);
    void action_remove_edge(const godot::StringName& p_from, int p_from_port, const godot::StringName& p_to, int p_to_port);

    // --- Tier 2: Direct Execution (The "Do" / "Undo" Targets) ---
    void _do_add_node(const godot::Ref<IdeamGraphNodeResource>& p_node);
    void _undo_add_node(const godot::StringName& p_node_name);
    void _do_add_edge(const godot::Dictionary& p_edge_data);
};

} // namespace ideam::godot_ext