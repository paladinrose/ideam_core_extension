#include "memory_graph_resource.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

namespace ideam::godot_ext {

void MemoryGraphResource::_bind_methods() {
    using namespace godot;
    
    ClassDB::bind_method(D_METHOD("set_volatile_requirement_capacity", "cap"), &MemoryGraphResource::set_volatile_requirement_capacity);
    ClassDB::bind_method(D_METHOD("get_volatile_requirement_capacity"), &MemoryGraphResource::get_volatile_requirement_capacity);
    
    // Exposed to the Inspector so the user can scale the Grant Registry Buffer
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volatile_requirement_capacity"), "set_volatile_requirement_capacity", "get_volatile_requirement_capacity");
}

void MemoryGraphResource::set_volatile_requirement_capacity(int p_cap) {
    volatile_requirement_capacity = p_cap;
    // Recalculate the master footprint if we are in volatile mode
    if (get_is_volatile()) update_managed_profiles();
    emit_changed();
}

void MemoryGraphResource::_append_managed_profiles(godot::TypedArray<ManagedBufferProfile>& r_profiles) const {
    // 1. Let the base IdeamGraphResource append the structural Nodes/Edges (SoA/AoS)
    IdeamGraphResource::_append_managed_profiles(r_profiles);

    // Hardware padding constraint
    constexpr int ALIGNMENT = 64; 

    // 2. Compute local capacities
    int node_cap = get_is_volatile() ? std::max(static_cast<int>(get_nodes().size()), get_volatile_node_capacity()) : static_cast<int>(get_nodes().size());

    // --- Profile 3: MemoryGraph Internal State Arrays ---
    // Represents: active_grants, node_metadata, and selection_metadata std::vectors inside MemoryGraphDOD
    int meta_bytes = node_cap * (sizeof(core::MemoryGrantPOD) + sizeof(core::MemoryNodeMetadata) + sizeof(core::SelectionMetadata));
    int padded_meta_bytes = (meta_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    godot::Ref<ManagedBufferProfile> meta_profile;
    meta_profile.instantiate();
    meta_profile->set_consumer_name(get_name());
    meta_profile->set_purpose("MemoryGraph State Arrays");
    meta_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::FLAT)); 
    meta_profile->set_alignment(ALIGNMENT);
    meta_profile->set_byte_footprint(padded_meta_bytes);
    r_profiles.append(meta_profile);

    // --- Profile 4: The Grant Registry Buffer ---
    // This is explicitly allocated *on the MemoryManager* as a PAGED buffer.
    // If we don't declare it here, the manager won't leave space for it in the Master Block.
    int req_cap = get_is_volatile() ? volatile_requirement_capacity : std::max(1024, volatile_requirement_capacity);
    int registry_bytes = req_cap * sizeof(core::GrantPartPOD);
    int padded_registry_bytes = (registry_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    godot::Ref<ManagedBufferProfile> registry_profile;
    registry_profile.instantiate();
    registry_profile->set_consumer_name(get_name());
    registry_profile->set_purpose("Grant Registry Buffer");
    
    // Explicitly casting the PAGED layout enum so the UI/Manager knows its backend type
    registry_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::PAGED)); 
    registry_profile->set_alignment(ALIGNMENT);
    registry_profile->set_byte_footprint(padded_registry_bytes);
    r_profiles.append(registry_profile);
}

std::shared_ptr<core::MemoryGraphDOD> MemoryGraphResource::compile_to_memory_graph(
    core::MemoryManagerDOD* p_manager, 
    godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const 
{
    // 1. Instantiate the specialized MemoryGraphDOD
    auto memory_graph = std::make_shared<core::MemoryGraphDOD>(p_manager);
    
    // The base get_nodes() now returns a strongly-typed Array of IdeamGraphNodeResource references
    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> current_nodes = get_nodes();
    godot::TypedArray<godot::Dictionary> current_edges = get_edges();

    memory_graph->reserve(current_nodes.size(), current_edges.size());
    r_ui_to_dod_map.clear();

    // 2. Compile Nodes (Fast Path via Direct Resource Pointers)
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<MemoryGraphNodeResource> n = current_nodes[i];
        if (!n.is_valid()) continue;

        godot::StringName ui_name = n->get_node_name();
        if (ui_name.is_empty()) continue; // Skip uninitialized identity

        // Retrieve native POD values straight from the VTable, no Dictionary map-hashing
        core::NodeID core_id = memory_graph->add_node(n->get_type_id());
        r_ui_to_dod_map[ui_name] = core_id;
    }

    // 3. Compile Edges (Remains Dict-based until Edge serialization is similarly refactored)
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        godot::StringName from_name = e["from"];
        godot::StringName to_name = e["to"];

        if (r_ui_to_dod_map.has(from_name) && r_ui_to_dod_map.has(to_name)) {
            core::NodeID from_id = r_ui_to_dod_map[from_name];
            core::NodeID to_id = r_ui_to_dod_map[to_name];

            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(e["from_port"]) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(e["to_port"]) : 0;

            memory_graph->connect_nodes(from_id, from_port, to_id, to_port);
        }
    }

    return memory_graph;
}

} // namespace ideam::godot_ext