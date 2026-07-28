#include "memory_graph_resource.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

namespace ideam::godot_ext {

void MemoryGraphResource::_bind_methods() {
    using namespace godot;
    
    ClassDB::bind_method(D_METHOD("set_meta_buffer", "profile"), &MemoryGraphResource::set_meta_buffer);
    ClassDB::bind_method(D_METHOD("get_meta_buffer"), &MemoryGraphResource::get_meta_buffer);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "meta_buffer", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferResource"), "set_meta_buffer", "get_meta_buffer");

    ClassDB::bind_method(D_METHOD("set_registry_buffer", "profile"), &MemoryGraphResource::set_registry_buffer);
    ClassDB::bind_method(D_METHOD("get_registry_buffer"), &MemoryGraphResource::get_registry_buffer);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "registry_buffer", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferResource"), "set_registry_buffer", "get_registry_buffer");

    ClassDB::bind_method(D_METHOD("set_volatile_requirement_capacity", "cap"), &MemoryGraphResource::set_volatile_requirement_capacity);
    ClassDB::bind_method(D_METHOD("get_volatile_requirement_capacity"), &MemoryGraphResource::get_volatile_requirement_capacity);
    
    // Exposed to the Inspector so the user can scale the Grant Registry Buffer
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volatile_requirement_capacity"), "set_volatile_requirement_capacity", "get_volatile_requirement_capacity");
}

void MemoryGraphResource::set_volatile_requirement_capacity(int p_cap) {
    if (p_cap == volatile_requirement_capacity) return;
    volatile_requirement_capacity = p_cap;
    // Recalculate the master footprint if we are in volatile mode
    if (get_is_volatile()) queue_update_managed_buffers();
    emit_changed();
}
int MemoryGraphResource::get_volatile_requirement_capacity() const { return volatile_requirement_capacity; }

void MemoryGraphResource::set_meta_buffer(const godot::Ref<ManagedBufferResource>& p_buffer) { 
    if(p_buffer == meta_buffer) return;
    meta_buffer = p_buffer; 
    emit_changed(); 
}
godot::Ref<ManagedBufferResource> MemoryGraphResource::get_meta_buffer() const { return meta_buffer; }

void MemoryGraphResource::set_registry_buffer(const godot::Ref<ManagedBufferResource>& p_buffer) { 
    if(registry_buffer == p_buffer) return;
    registry_buffer = p_buffer; 
    emit_changed(); 
}
godot::Ref<ManagedBufferResource> MemoryGraphResource::get_registry_buffer() const { return registry_buffer; }

void MemoryGraphResource::_ensure_managed_buffers() {
    // 1. Let the base IdeamGraphResource instantiate/update the structural Nodes/Edges (SoA/AoS)
    IdeamGraphResource::_ensure_managed_buffers();

    // Hardware padding constraint
    constexpr int ALIGNMENT = 64; 

    // 2. Compute local capacities
    int node_cap = get_is_volatile() ? std::max(static_cast<int>(get_nodes().size()), get_volatile_node_capacity()) : static_cast<int>(get_nodes().size());

    // --- Buffer Profile 3: MemoryGraph Internal State Arrays ---
    int meta_bytes = node_cap * (sizeof(core::MemoryGrantPOD) + sizeof(core::MemoryNodeMetadata) + sizeof(core::SelectionMetadata));
    int padded_meta_bytes = (meta_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    if (meta_buffer.is_null()) {
        meta_buffer.instantiate();
        meta_buffer->set_purpose("MemoryGraph State Arrays");
        meta_buffer->set_layout_type(static_cast<int>(core::BufferLayoutType::FLAT)); 
        meta_buffer->set_alignment(ALIGNMENT);
    }
    
    // Always update dynamically changing values
    meta_buffer->set_consumer_name(get_consumer_key());
    meta_buffer->set_byte_footprint(padded_meta_bytes);

    // --- Buffer Profile 4: The Grant Registry Buffer ---
    int req_cap = get_is_volatile() ? volatile_requirement_capacity : std::max(1024, volatile_requirement_capacity);
    int registry_bytes = req_cap * sizeof(core::GrantPartPOD);
    int padded_registry_bytes = (registry_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    if (registry_buffer.is_null()) {
        registry_buffer.instantiate();
        registry_buffer->set_purpose("Grant Registry Buffer");
        registry_buffer->set_layout_type(static_cast<int>(core::BufferLayoutType::PAGED)); 
        registry_buffer->set_alignment(ALIGNMENT);
    }
    
    // Always update dynamically changing values
    registry_buffer->set_consumer_name(get_consumer_key());
    registry_buffer->set_byte_footprint(padded_registry_bytes);
}

void MemoryGraphResource::_gather_managed_buffers(godot::TypedArray<ManagedBufferResource>& r_buffers) const {
    // 1. Pack the base topology profiles (node_buffer, edge_buffer)
    IdeamGraphResource::_gather_managed_buffers(r_buffers);

    // 2. Pack the specific Memory Graph profiles
    if (meta_buffer.is_valid()) r_buffers.append(meta_buffer);
    if (registry_buffer.is_valid()) r_buffers.append(registry_buffer);
}

godot::Ref<MemoryGraphNodeResource> MemoryGraphResource::_get_node_by_name(const godot::StringName& p_name) const {
    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> current_nodes = get_nodes();
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<MemoryGraphNodeResource> n = current_nodes[i];
        if (n.is_valid() && n->get_node_name() == p_name) {
            return n;
        }
    }
    return godot::Ref<MemoryGraphNodeResource>();
}

// Update the compilation method
std::shared_ptr<core::MemoryGraphDOD> MemoryGraphResource::compile_to_memory_graph(
    core::MemoryManagerDOD* p_manager, 
    godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const 
{
    auto memory_graph = std::make_shared<core::MemoryGraphDOD>(p_manager);
    
    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> current_nodes = get_nodes();
    godot::TypedArray<godot::Dictionary> current_edges = get_edges();

    memory_graph->reserve(current_nodes.size(), current_edges.size());
    r_ui_to_dod_map.clear();

    // 2. Compile Nodes & Push DOD Requirements
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<MemoryGraphNodeResource> n = current_nodes[i];
        if (!n.is_valid()) continue;

        godot::StringName ui_name = n->get_node_name();
        if (ui_name.is_empty()) continue; 

        core::NodeID core_id = memory_graph->add_node(n->get_type_id());
        r_ui_to_dod_map[ui_name] = core_id;

        // --- DOD Handshake: Independent Grants ---
        if (n->get_derivation_mode() == MemoryGraphNodeResource::MODE_INDEPENDENT) {
            godot::Ref<MemoryGrantResource> grant_res = n->get_memory_grant();
            if (grant_res.is_valid()) {
                godot::TypedArray<GrantPartResource> ui_parts = grant_res->get_configured_parts();
                
                // Stack-allocate a fast vector for the staging push
                std::vector<core::GrantPartPOD> dod_parts;
                dod_parts.reserve(ui_parts.size());
                
                for (int p = 0; p < ui_parts.size(); ++p) {
                    godot::Ref<GrantPartResource> part_res = ui_parts[p];
                    if (part_res.is_valid()) {
                        core::GrantPartPOD pod_part{};
                        pod_part.buffer_id = part_res->get_buffer_id();
                        pod_part.element_stride = part_res->get_element_stride();
                        // Assuming 0=READ, 1=WRITE maps to your internal enums
                        pod_part.access_mode = static_cast<core::BufferAccessMode>(part_res->get_access_mode());
                        
                        if (part_res->get_is_contiguous()) {
                            pod_part.selection.mode = core::SelectionMode::RANGE;
                        }
                        dod_parts.push_back(pod_part);
                    }
                }
                
                if (!dod_parts.empty()) {
                    memory_graph->set_node_requirements(core_id, dod_parts);
                }
            }
        }
    }

    // 3. Compile Edges & Enforce Fork Dependencies
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        godot::StringName from_name = e["from"];
        godot::StringName to_name = e["to"];

        if (r_ui_to_dod_map.has(from_name) && r_ui_to_dod_map.has(to_name)) {
            core::NodeID from_id = r_ui_to_dod_map[from_name];
            core::NodeID to_id = r_ui_to_dod_map[to_name];

            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(static_cast<int64_t>(e["from_port"])) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(static_cast<int64_t>(e["to_port"])) : 0;

            memory_graph->connect_nodes(from_id, from_port, to_id, to_port);

            // --- DOD Handshake: Forked Grants ---
            // If the target node is flagged to inherit memory pointers, we issue the fork 
            // strictly along the compiled topological edge.
            godot::Ref<MemoryGraphNodeResource> to_res = _get_node_by_name(to_name);
            if (to_res.is_valid() && to_res->get_derivation_mode() == MemoryGraphNodeResource::MODE_FORKED) {
                memory_graph->fork_grant(from_id, to_id);
            }
        }
    }

    return memory_graph;
}

} // namespace ideam::godot_ext