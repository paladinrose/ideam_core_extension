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
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_ARRAY_TYPE, "IdeamGraphNodeResource"), "set_nodes", "get_nodes");
    
    ClassDB::bind_method(D_METHOD("set_edges", "edges"), &IdeamGraphResource::set_edges);
    ClassDB::bind_method(D_METHOD("get_edges"), &IdeamGraphResource::get_edges);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "edges", PROPERTY_HINT_NONE, ""), "set_edges", "get_edges");

    ClassDB::bind_method(D_METHOD("set_groups", "groups"), &IdeamGraphResource::set_groups);
    ClassDB::bind_method(D_METHOD("get_groups"), &IdeamGraphResource::get_groups);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "groups", PROPERTY_HINT_ARRAY_TYPE, "IdeamGraphGroupResource"), "set_groups", "get_groups");

    ClassDB::bind_method(D_METHOD("set_is_volatile", "is_volatile"), &IdeamGraphResource::set_is_volatile);
    ClassDB::bind_method(D_METHOD("get_is_volatile"), &IdeamGraphResource::get_is_volatile);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_volatile_at_runtime"), "set_is_volatile", "get_is_volatile");

    ClassDB::bind_method(D_METHOD("set_volatile_node_capacity", "cap"), &IdeamGraphResource::set_volatile_node_capacity);
    ClassDB::bind_method(D_METHOD("get_volatile_node_capacity"), &IdeamGraphResource::get_volatile_node_capacity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volatile_node_capacity"), "set_volatile_node_capacity", "get_volatile_node_capacity");

    ClassDB::bind_method(D_METHOD("set_volatile_edge_capacity", "cap"), &IdeamGraphResource::set_volatile_edge_capacity);
    ClassDB::bind_method(D_METHOD("get_volatile_edge_capacity"), &IdeamGraphResource::get_volatile_edge_capacity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volatile_edge_capacity"), "set_volatile_edge_capacity", "get_volatile_edge_capacity");

    ClassDB::bind_method(godot::D_METHOD("_get_node_dependencies", "node"), &IdeamGraphResource::_get_node_dependencies);
    ClassDB::bind_method(godot::D_METHOD("get_execution_waves"), &IdeamGraphResource::get_execution_waves);

    ClassDB::bind_method(D_METHOD("set_consumer_key", "key"), &IdeamGraphResource::set_consumer_key);
    ClassDB::bind_method(D_METHOD("get_consumer_key"), &IdeamGraphResource::get_consumer_key);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "consumer_key"), "set_consumer_key", "get_consumer_key");
    
    ClassDB::bind_method(D_METHOD("set_node_profile", "profile"), &IdeamGraphResource::set_node_profile);
    ClassDB::bind_method(D_METHOD("get_node_profile"), &IdeamGraphResource::get_node_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "node_profile", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferProfile"), "set_node_profile", "get_node_profile");

    ClassDB::bind_method(D_METHOD("set_edge_profile", "profile"), &IdeamGraphResource::set_edge_profile);
    ClassDB::bind_method(D_METHOD("get_edge_profile"), &IdeamGraphResource::get_edge_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "edge_profile", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferProfile"), "set_edge_profile", "get_edge_profile");

    ClassDB::bind_method(D_METHOD("queue_update_managed_profiles"), &IdeamGraphResource::queue_update_managed_profiles);
    ClassDB::bind_method(D_METHOD("update_managed_profiles"), &IdeamGraphResource::update_managed_profiles);

    // Execution methods
    ClassDB::bind_method(D_METHOD("_do_add_node", "node"), &IdeamGraphResource::_do_add_node);
    ClassDB::bind_method(D_METHOD("_undo_add_node", "node_name"), &IdeamGraphResource::_undo_add_node);
    ClassDB::bind_method(D_METHOD("_do_add_edge", "edge_data"), &IdeamGraphResource::_do_add_edge);
}

// --- Property Setters ---
// Every structural change immediately synchronizes the memory footprint.

void IdeamGraphResource::set_memory_manager(const godot::Ref<MemoryManagerResource>& p_manager) { 
    if (memory_manager == p_manager) return;
    memory_manager = p_manager; 
    queue_update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_nodes(const godot::TypedArray<godot::Ref<IdeamGraphNodeResource>>& p_nodes) { 
    if (nodes == p_nodes) return;
    nodes = p_nodes; 
    queue_update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_edges(const godot::TypedArray<godot::Dictionary>& p_edges) { 
    if (edges == p_edges) return;
    edges = p_edges; 
    queue_update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_groups(const godot::TypedArray<godot::Ref<IdeamGraphGroupResource>>& p_groups) {
    if (groups == p_groups) return;
    groups = p_groups;
    emit_changed();
}

void IdeamGraphResource::set_is_volatile(bool p_volatile) { 
    if (is_volatile_at_runtime == p_volatile) return;
    is_volatile_at_runtime = p_volatile; 
    if (is_volatile_at_runtime) queue_update_managed_profiles();
    emit_changed(); 
}

void IdeamGraphResource::set_volatile_node_capacity(int p_cap) {
    if (volatile_node_capacity == p_cap) return;
    volatile_node_capacity = p_cap;
    if (is_volatile_at_runtime) queue_update_managed_profiles();
    emit_changed();
}

void IdeamGraphResource::set_volatile_edge_capacity(int p_cap) {
    if (volatile_edge_capacity == p_cap) return;
    volatile_edge_capacity = p_cap;
    if (is_volatile_at_runtime) queue_update_managed_profiles();
    emit_changed();
}

// --- Managed Profiles Handshake ---
void IdeamGraphResource::queue_update_managed_profiles() {
    // This prevents the update from running 10 times if 10 properties are loaded at once.
    if (!is_update_queued) {
        is_update_queued = true;
        call_deferred("update_managed_profiles"); 
    }
}

void IdeamGraphResource::update_managed_profiles() {
    // 1. Ensure internal explicit profiles exist and have the latest footprint data.
    _ensure_managed_profiles();

    if (memory_manager.is_null()) return;

    // 2. Pack the explicit members into a transport array via the virtual hook.
    godot::TypedArray<ManagedBufferProfile> profiles_to_send;
    _gather_managed_profiles(profiles_to_send);

    // 3. Send the packaged array to the manager
    memory_manager->register_consumer_buffers(get_consumer_key(), profiles_to_send);
    
    is_update_queued = false; // Reset the flag after the update is complete
}

void IdeamGraphResource::set_consumer_key(const godot::String& p_key) {
    // First, we want to clear any existing buffers associated with the old consumer key.
    memory_manager->clear_consumer_buffers(consumer_key);

    consumer_key = p_key;
    
    // The queued update will update the consumer_name of each profile, then re-register them with the MemoryManagerResource.
    queue_update_managed_profiles();
    
    emit_changed();
}

godot::String IdeamGraphResource::get_consumer_key() {
    if (!consumer_key.is_empty() && consumer_key != "IdeamGraph_Anonymous") return consumer_key;
    
    // Use the Resource's path if available, otherwise fallback to its name.
    consumer_key = get_path().is_empty() ? get_name() : get_path();
    if (consumer_key.is_empty()) consumer_key = "IdeamGraph_Anonymous";

    return consumer_key;
}

void IdeamGraphResource::_ensure_managed_profiles(){
    // 1. Calculate the required bounds
    int node_cap = is_volatile_at_runtime ? std::max(static_cast<int>(nodes.size()), volatile_node_capacity) : static_cast<int>(nodes.size());
    int edge_cap = is_volatile_at_runtime ? std::max(static_cast<int>(edges.size()), volatile_edge_capacity) : static_cast<int>(edges.size());

    constexpr int ALIGNMENT = 64; 

    // --- Profile 1: Topology Nodes (SoA) ---
    int node_bytes = node_cap * 12;
    int padded_node_bytes = (node_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    if (node_profile.is_null()) {
        node_profile.instantiate();
        node_profile->set_purpose("Topology Nodes SoA");
        node_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::SOA));
        node_profile->set_alignment(ALIGNMENT);
    }
    
    // Always update dynamically changing values
    node_profile->set_consumer_name(get_consumer_key());
    node_profile->set_byte_footprint(padded_node_bytes);

    // --- Profile 2: Topology Edges (AoS) ---
    int edge_bytes = edge_cap * sizeof(core::GraphEdgeData);
    int padded_edge_bytes = (edge_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    if (edge_profile.is_null()) {
        edge_profile.instantiate();
        edge_profile->set_purpose("Topology Edges AoS");
        edge_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::AOS));
        edge_profile->set_alignment(ALIGNMENT);
    }
    
    // Always update dynamically changing values
    edge_profile->set_consumer_name(get_consumer_key());
    edge_profile->set_byte_footprint(padded_edge_bytes);
}

void IdeamGraphResource::_gather_managed_profiles(godot::TypedArray<ManagedBufferProfile>& r_profiles) const {
    if (node_profile.is_valid()) r_profiles.append(node_profile);
    if (edge_profile.is_valid()) r_profiles.append(edge_profile);
}

godot::TypedArray<godot::StringName> IdeamGraphResource::_get_node_dependencies(const godot::StringName& p_node) const {
    godot::TypedArray<godot::StringName> dependencies;
    
    // Base implementation: Any incoming edge is a blocking dependency.
    for (int i = 0; i < edges.size(); ++i) {
        godot::Dictionary e = edges[i];
        if (e.has("to") && e.has("from")) {
            if (e["to"] == godot::Variant(p_node)) {
                dependencies.push_back(e["from"]);
            }
        }
    }
    return dependencies;
}

// Update this signature in both ideam_graph_resource.h and ideam_graph_resource.cpp
godot::Array IdeamGraphResource::get_execution_waves() const {
    godot::Array execution_waves; // Changed from TypedArray<TypedArray<StringName>>
    
    if (nodes.is_empty()) return execution_waves;

    // 1. Build rapid-access structures
    godot::HashMap<godot::StringName, godot::Ref<IdeamGraphNodeResource>> node_map;
    godot::HashMap<godot::StringName, int> in_degree;
    godot::HashMap<godot::StringName, std::vector<godot::StringName>> adj_list;

    for (int i = 0; i < nodes.size(); ++i) {
        godot::Ref<IdeamGraphNodeResource> n = nodes[i];
        if (n.is_valid() && !n->get_node_name().is_empty()) {
            godot::StringName name = n->get_node_name();
            node_map[name] = n;
            in_degree[name] = 0; 
            adj_list[name] = std::vector<godot::StringName>();
        }
    }

    // 2. Populate dependencies via the virtual hook
    for (const auto& kv : node_map) {
        godot::StringName target_node = kv.key;
        godot::TypedArray<godot::StringName> deps = _get_node_dependencies(target_node);
        
        for (int i = 0; i < deps.size(); ++i) {
            godot::StringName dep_node = deps[i];
            if (node_map.has(dep_node)) {
                adj_list[dep_node].push_back(target_node);
                in_degree[target_node]++;
            }
        }
    }

    // 3. Collect the Entry Wave (In-degree == 0)
    std::vector<godot::StringName> current_wave;
    for (const auto& kv : in_degree) {
        if (kv.value == 0) {
            current_wave.push_back(kv.key);
        }
    }

    // 4. Kahn Wave Processing & Priority Sorting
    while (!current_wave.empty()) {
        
        // --- DOD Mirror: Intra-Wave Priority Sort ---
        std::sort(current_wave.begin(), current_wave.end(), [&node_map](const godot::StringName& a, const godot::StringName& b) {
            int32_t priority_a = node_map[a]->get_execution_priority();
            int32_t priority_b = node_map[b]->get_execution_priority();
            return priority_a > priority_b; // Descending order
        });

        // Pack the sorted C++ vector into a TypedArray boundary for safety on the inside
        godot::TypedArray<godot::StringName> wave_array;
        for (const auto& node_name : current_wave) {
            wave_array.push_back(node_name);
        }
        // Implicitly casts TypedArray<StringName> to Variant/Array element
        execution_waves.push_back(wave_array);

        // Advance the topological front
        std::vector<godot::StringName> next_wave;
        for (const auto& u : current_wave) {
            for (const auto& v : adj_list[u]) {
                in_degree[v]--;
                if (in_degree[v] == 0) {
                    next_wave.push_back(v);
                }
            }
        }
        
        current_wave = std::move(next_wave);
    }

    return execution_waves;
}

// --- Tier 1: Action Routers (Called by UI) ---

void IdeamGraphResource::action_add_node(const godot::Ref<IdeamGraphNodeResource>& p_node) {
    _do_add_node(p_node);
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

void IdeamGraphResource::action_create_group(const godot::Ref<IdeamGraphGroupResource>& p_group) {
    if (p_group.is_valid()) {
        groups.append(p_group);
        emit_changed();
    }
}

void IdeamGraphResource::action_remove_group(const godot::StringName& p_group_name) {
    for (int i = 0; i < groups.size(); ++i) {
        godot::Ref<IdeamGraphGroupResource> g = groups[i];
        if (g.is_valid() && g->get_group_name() == p_group_name) {
            groups.remove_at(i);
            emit_changed();
            break;
        }
    }
}

void IdeamGraphResource::action_attach_to_group(const godot::StringName& p_group_name, const godot::StringName& p_node_name) {
    for (int i = 0; i < groups.size(); ++i) {
        godot::Ref<IdeamGraphGroupResource> g = groups[i];
        if (g.is_valid() && g->get_group_name() == p_group_name) {
            godot::TypedArray<godot::StringName> g_nodes = g->get_nodes();
            if (!g_nodes.has(p_node_name)) {
                g_nodes.append(p_node_name);
                g->set_nodes(g_nodes);
                emit_changed();
            }
            break;
        }
    }
}

void IdeamGraphResource::action_detach_from_group(const godot::StringName& p_group_name, const godot::StringName& p_node_name) {
    for (int i = 0; i < groups.size(); ++i) {
        godot::Ref<IdeamGraphGroupResource> g = groups[i];
        if (g.is_valid() && g->get_group_name() == p_group_name) {
            godot::TypedArray<godot::StringName> g_nodes = g->get_nodes();
            int idx = g_nodes.find(p_node_name);
            if (idx != -1) {
                g_nodes.remove_at(idx);
                g->set_nodes(g_nodes);
                emit_changed();
            }
            break;
        }
    }
}

// --- Tier 2: Direct Execution (The "Do" / "Undo" Targets) ---

void IdeamGraphResource::_do_add_node(const godot::Ref<IdeamGraphNodeResource>& p_node) {
    if (p_node.is_valid()) {
        nodes.append(p_node);
        p_node->set_local_to_scene(true); // Ensures the node gets saved with the scene if it's not already a sub-resource
        update_managed_profiles(); // Ensures memory UI updates instantly
        emit_changed();
    }
}

void IdeamGraphResource::_undo_add_node(const godot::StringName& p_node_name) {
    for (int i = 0; i < nodes.size(); ++i) {
        godot::Ref<IdeamGraphNodeResource> n = nodes[i];
        if (n.is_valid() && n->get_node_name() == p_node_name) {
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