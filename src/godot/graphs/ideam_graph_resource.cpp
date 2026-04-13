#include "ideam_graph_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm> // For std::max

namespace ideam::godot_ext {

void IdeamGraphResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager", "manager"), &IdeamGraphResource::set_memory_manager);
    ClassDB::bind_method(D_METHOD("get_memory_manager"), &IdeamGraphResource::get_memory_manager);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "memory_manager", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"), "set_memory_manager", "get_memory_manager");

    ClassDB::bind_method(D_METHOD("set_nodes", "nodes"), &IdeamGraphResource::set_nodes);
    ClassDB::bind_method(D_METHOD("get_nodes"), &IdeamGraphResource::get_nodes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_nodes", "get_nodes");
    
    ClassDB::bind_method(D_METHOD("set_edges", "edges"), &IdeamGraphResource::set_edges);
    ClassDB::bind_method(D_METHOD("get_edges"), &IdeamGraphResource::get_edges);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "edges", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_edges", "get_edges");

    ClassDB::bind_method(D_METHOD("set_is_volatile", "is_volatile"), &IdeamGraphResource::set_is_volatile);
    ClassDB::bind_method(D_METHOD("get_is_volatile"), &IdeamGraphResource::get_is_volatile);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_volatile_at_runtime"), "set_is_volatile", "get_is_volatile");

    ClassDB::bind_method(D_METHOD("set_volatile_node_capacity", "cap"), &IdeamGraphResource::set_volatile_node_capacity);
    ClassDB::bind_method(D_METHOD("get_volatile_node_capacity"), &IdeamGraphResource::get_volatile_node_capacity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volatile_node_capacity"), "set_volatile_node_capacity", "get_volatile_node_capacity");

    ClassDB::bind_method(D_METHOD("set_volatile_edge_capacity", "cap"), &IdeamGraphResource::set_volatile_edge_capacity);
    ClassDB::bind_method(D_METHOD("get_volatile_edge_capacity"), &IdeamGraphResource::get_volatile_edge_capacity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volatile_edge_capacity"), "set_volatile_edge_capacity", "get_volatile_edge_capacity");

    // Execution methods bound below (unchanged)...
    ClassDB::bind_method(D_METHOD("_do_add_node", "node_data"), &IdeamGraphResource::_do_add_node);
    ClassDB::bind_method(D_METHOD("_undo_add_node", "node_name"), &IdeamGraphResource::_undo_add_node);
    ClassDB::bind_method(D_METHOD("_do_add_edge", "edge_data"), &IdeamGraphResource::_do_add_edge);
}

// --- Property Setters ---
// Every structural change immediately synchronizes the memory footprint.

void IdeamGraphResource::set_memory_manager(const godot::Ref<MemoryManagerResource>& p_manager) { 
    memory_manager = p_manager; 
    update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_nodes(const godot::TypedArray<godot::Dictionary>& p_nodes) { 
    nodes = p_nodes; 
    update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_edges(const godot::TypedArray<godot::Dictionary>& p_edges) { 
    edges = p_edges; 
    update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_is_volatile(bool p_volatile) { 
    is_volatile_at_runtime = p_volatile; 
    update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_volatile_node_capacity(int p_cap) {
    volatile_node_capacity = p_cap;
    if (is_volatile_at_runtime) update_managed_profiles();
    emit_changed();
}

void IdeamGraphResource::set_volatile_edge_capacity(int p_cap) {
    volatile_edge_capacity = p_cap;
    if (is_volatile_at_runtime) update_managed_profiles();
    emit_changed();
}

// --- The Handshake ---

void IdeamGraphResource::update_managed_profiles() {
    if (memory_manager.is_null()) return;

    godot::TypedArray<ManagedBufferProfile> profiles;
    
    // Calls the local (or derived class's overridden) profile generator
    _append_managed_profiles(profiles);

    // Provide the consumer name (we use the Resource's path or name as a unique key)
    godot::StringName consumer_key = get_path().is_empty() ? get_name() : get_path();
    if (consumer_key.is_empty()) consumer_key = "IdeamGraph_Anonymous";

    memory_manager->register_consumer_buffers(consumer_key, profiles);
}

void IdeamGraphResource::_append_managed_profiles(godot::TypedArray<ManagedBufferProfile>& r_profiles) const {
    // 1. Calculate the required bounds
    int node_cap = is_volatile_at_runtime ? std::max(static_cast<int>(nodes.size()), volatile_node_capacity) : static_cast<int>(nodes.size());
    int edge_cap = is_volatile_at_runtime ? std::max(static_cast<int>(edges.size()), volatile_edge_capacity) : static_cast<int>(edges.size());

    // Hardware padding constraint
    constexpr int ALIGNMENT = 64; 

    // --- Profile 1: Topology Nodes (SoA) ---
    // Represents: BuildNodesSoA { vector<NodeID>, vector<uint32_t>, vector<int32_t> }
    // NodeID (4) + type_id (4) + execution_priority (4) = 12 bytes per node
    int node_bytes = node_cap * 12;
    int padded_node_bytes = (node_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    godot::Ref<ManagedBufferProfile> node_profile;
    node_profile.instantiate();
    node_profile->set_consumer_name(get_name());
    node_profile->set_purpose("Topology Nodes SoA");
    node_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::SOA)); // 4
    node_profile->set_alignment(ALIGNMENT);
    node_profile->set_byte_footprint(padded_node_bytes);
    r_profiles.append(node_profile);

    // --- Profile 2: Topology Edges (AoS) ---
    // Represents: vector<GraphEdgeData>
    int edge_bytes = edge_cap * sizeof(core::GraphEdgeData);
    int padded_edge_bytes = (edge_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    godot::Ref<ManagedBufferProfile> edge_profile;
    edge_profile.instantiate();
    edge_profile->set_consumer_name(get_name());
    edge_profile->set_purpose("Topology Edges AoS");
    edge_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::AOS)); // 2
    edge_profile->set_alignment(ALIGNMENT);
    edge_profile->set_byte_footprint(padded_edge_bytes);
    r_profiles.append(edge_profile);
}

// --- Tier 1: Action Routers (Called by UI) ---

void IdeamGraphResource::action_add_node(const godot::Dictionary& p_node_data) {
    _do_add_node(p_node_data);
}

void IdeamGraphResource::action_remove_node(const godot::StringName& p_node_name) {
    _undo_add_node(p_node_name);
}

void IdeamGraphResource::action_add_edge(const godot::Dictionary& p_edge_data) {
    _do_add_edge(p_edge_data);
}

void IdeamGraphResource::action_remove_edge(const godot::StringName& p_from, int p_from_port, const godot::StringName& p_to, int p_to_port) {
    for (int i = 0; i < edges.size(); ++i) {
        godot::Dictionary e = edges[i];
        if (e["from"] == godot::Variant(p_from) && static_cast<int>(e["from_port"]) == p_from_port &&
            e["to"] == godot::Variant(p_to) && static_cast<int>(e["to_port"]) == p_to_port) {
            edges.remove_at(i);
            update_managed_profiles();
            emit_changed();
            break;
        }
    }
}

// --- Tier 2: Direct Execution (The "Do" / "Undo" Targets) ---

void IdeamGraphResource::_do_add_node(const godot::Dictionary& p_node_data) {
    nodes.append(p_node_data);
    update_managed_profiles(); // Ensures memory UI updates instantly
    emit_changed();
}

void IdeamGraphResource::_undo_add_node(const godot::StringName& p_node_name) {
    for (int i = 0; i < nodes.size(); ++i) {
        godot::Dictionary n = nodes[i];
        if (n.has("name") && static_cast<godot::StringName>(n["name"]) == p_node_name) {
            nodes.remove_at(i);
            update_managed_profiles(); // Ensures memory UI updates instantly
            emit_changed();
            break;
        }
    }
}

void IdeamGraphResource::_do_add_edge(const godot::Dictionary& p_edge_data) {
    edges.append(p_edge_data);
    update_managed_profiles(); // Ensures memory UI updates instantly
    emit_changed();
}

} // namespace ideam::godot_ext