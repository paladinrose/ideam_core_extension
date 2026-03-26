#include "task_graph_dod.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>

namespace ideam::core {

TaskGraphDOD::TaskGraphDOD(MemoryManagerDOD* p_manager) : MemoryGraphDOD(p_manager) {
    sync_with_manager();
}

NodeID TaskGraphDOD::add_task_node(TaskTypeDOD p_type) {
    NodeID id = add_node(static_cast<uint32_t>(p_type));
    
    if (id >= task_types.size()) {
        task_types.resize(id + 1);
        cpu_metadata.resize(id + 1);
        gpu_metadata.resize(id + 1);
        input_port_map.resize(id + 1);
        output_port_map.resize(id + 1);
        port_constants.resize(id + 1);
        baked_connections.resize(id + 1);
    }

    task_types[id] = p_type;
    return id;
}

void TaskGraphDOD::configure_cpu_task(NodeID p_id, godot::Object* p_target, const std::string& p_method) {
    if (p_id < cpu_metadata.size()) {
        cpu_metadata[p_id].reflection_target = p_target;
        cpu_metadata[p_id].execution_method = p_method;
    }
}

void TaskGraphDOD::configure_native_interface(NodeID p_id, INativeTask* p_interface) {
    if (p_id < cpu_metadata.size()) {
        cpu_metadata[p_id].native_interface = static_cast<void*>(p_interface);
    }
}

void TaskGraphDOD::configure_gpu_task(NodeID p_id, godot::RID p_pipeline, uint32_t x, uint32_t y, uint32_t z) {
    if (p_id < gpu_metadata.size()) {
        gpu_metadata[p_id].pipeline_rid = p_pipeline;
        gpu_metadata[p_id].dispatch_x = x;
        gpu_metadata[p_id].dispatch_y = y;
        gpu_metadata[p_id].dispatch_z = z;
    }
}

void TaskGraphDOD::set_port_mapping(NodeID p_id, bool p_input, uint32_t p_port_idx, DataType p_type, uint32_t p_buffer_id) {
    auto& map = p_input ? input_port_map[p_id] : output_port_map[p_id];
    if (p_port_idx >= map.size()) map.resize(p_port_idx + 1);
    
    map[p_port_idx].type = p_type;
    map[p_port_idx].buffer_id = p_buffer_id;
    map[p_port_idx].byte_size = MemoryUtilities::get_type_byte_size(p_type, BufferAlignmentMode::TIGHT);
    
    dirty_flags |= RESOURCES;
}

void TaskGraphDOD::set_port_constant(NodeID p_id, uint32_t p_port_idx, const godot::Variant& p_value) {
    if (p_id >= port_constants.size()) return;
    if (p_port_idx >= port_constants[p_id].size()) port_constants[p_id].resize(p_port_idx + 1);
    port_constants[p_id][p_port_idx] = p_value;
}

MemoryGrantPOD* TaskGraphDOD::get_grant_mutable(NodeID p_id) {
    if (p_id < active_grants.size() && active_grants[p_id].active) {
        return &active_grants[p_id];
    }
    return nullptr;
}

void TaskGraphDOD::execute_graph_dod(double p_delta) {
    if (!manager) return;
    
    if (!rd) {
        sync_with_manager();
    }

    // Step 1: Ensure physical memory grants are valid and pointers are fresh
    validate_grants();

    if (dirty_flags & RESOURCES) {
        _bake_port_connections();
        dirty_flags &= ~RESOURCES;
    }

    manager->flush_gpu_updates();

    // Access wave topology
    MemoryGrantPOD wave_grant;
    if (!manager->bake_grant(wave_grant, {
        {.buffer_id = wave_node_id, .access_mode = BufferAccessMode::READ},
        {.buffer_id = wave_meta_id, .access_mode = BufferAccessMode::READ}
    })) return;

    NodeID* all_nodes = reinterpret_cast<NodeID*>(wave_grant.parts[0].raw_base_ptr);
    WaveInfo* wave_meta = reinterpret_cast<WaveInfo*>(wave_grant.parts[1].raw_base_ptr);
    
    // Derive wave count from buffer size
    const MemoryBufferPOD* m_buf = manager->get_buffer(wave_meta_id);
    uint32_t wave_count = (m_buf) ? static_cast<uint32_t>(m_buf->capacity_bytes / sizeof(WaveInfo)) : 0;

    for (uint32_t w = 0; w < wave_count; ++w) {
        NodeID* wave_nodes = all_nodes + wave_meta[w].offset;
        uint32_t count = wave_meta[w].count;

        // Step 2: Clean selections for the whole wave before execution
        for (uint32_t i = 0; i < count; ++i) {
            _clean_selections(wave_nodes[i]);
        }

        // 3-Phase Process
        _batch_setup_wave(wave_nodes, count);
        _batch_execute_wave(wave_nodes, count, p_delta);
        _batch_resolve_wave(wave_nodes, count);
    }

    manager->release_grant(wave_grant);
}

void TaskGraphDOD::_clean_selections(NodeID p_id) {
    SelectionMetadata* meta = get_selection_meta(p_id);
    if (!meta || meta->dirty_parts_mask == 0) return;

    MemoryGrantPOD* grant = get_grant_mutable(p_id);
    if (!grant) return;

    TaskTypeDOD type = task_types[p_id];
    
    // Only Native CPU or Query Culler tasks can perform selection cleaning
    if (type == TaskTypeDOD::NATIVE_CPU || type == TaskTypeDOD::QUERY_CULLER) {
        TaskCPUMetadata& cpu_meta = cpu_metadata[p_id];
        if (cpu_meta.native_interface) {
            INativeTask* task = static_cast<INativeTask*>(cpu_meta.native_interface);
            
            // Phase 0: Culling. The task uses dirty_parts_mask to know which 
            // GrantParts' MemoryBufferSelectionPODs need refinement.
            TaskContextPOD ctx{ 0.0, grant }; 
            task->cull_selections(ctx, meta->dirty_parts_mask);
        }
    }

    // Mark as clean so we don't re-run query kernels until next fork or invalidation
    meta->dirty_parts_mask = 0;
}

void TaskGraphDOD::_bake_port_connections() {
    for (auto& vec : baked_connections) vec.clear();

    for (const auto& edge : build_edges) {
        if (edge.id == INVALID_ID) continue;

        const MemoryGrantPOD* src_grant = get_grant(edge.from_node);
        const MemoryGrantPOD* dst_grant = get_grant(edge.to_node);
        if (!src_grant || !dst_grant) continue;

        if (edge.from_port >= output_port_map[edge.from_node].size() || 
            edge.to_port >= input_port_map[edge.to_node].size()) continue;

        TaskPortMetadata& out_meta = output_port_map[edge.from_node][edge.from_port];
        TaskPortMetadata& in_meta = input_port_map[edge.to_node][edge.to_port];

        TaskPortConnectionDOD conn;
        conn.byte_size = in_meta.byte_size;

        for (uint32_t p = 0; p < src_grant->part_count; ++p) {
            if (src_grant->parts[p].buffer_id == out_meta.buffer_id) {
                conn.src_ptr = src_grant->parts[p].raw_base_ptr;
                break;
            }
        }

        for (uint32_t p = 0; p < dst_grant->part_count; ++p) {
            if (dst_grant->parts[p].buffer_id == in_meta.buffer_id) {
                conn.dst_ptr = dst_grant->parts[p].raw_base_ptr;
                break;
            }
        }

        if (conn.src_ptr && conn.dst_ptr) {
            baked_connections[edge.to_node].push_back(conn);
        }
    }
}

void TaskGraphDOD::_batch_setup_wave(const NodeID* p_nodes, uint32_t p_count) {
    for (uint32_t i = 0; i < p_count; ++i) {
        NodeID id = p_nodes[i];
        
        for (const auto& conn : baked_connections[id]) {
            std::memcpy(conn.dst_ptr, conn.src_ptr, conn.byte_size);
        }

        const MemoryGrantPOD* grant = get_grant(id);
        if (!grant) continue;

        const auto& ports = input_port_map[id];
        const auto& constants = port_constants[id];

        for (uint32_t p_idx = 0; p_idx < ports.size(); ++p_idx) {
            if (p_idx >= constants.size() || constants[p_idx].get_type() == godot::Variant::NIL) continue;
            
            for (uint32_t g_part = 0; g_part < grant->part_count; ++g_part) {
                if (grant->parts[g_part].buffer_id == ports[p_idx].buffer_id) {
                    _variant_to_raw(constants[p_idx], grant->parts[g_part].raw_base_ptr, ports[p_idx].type);
                    break;
                }
            }
        }
    }
}

void TaskGraphDOD::_batch_execute_wave(const NodeID* p_nodes, uint32_t p_count, double p_delta) {
    int64_t compute_list = -1;

    for (uint32_t i = 0; i < p_count; ++i) {
        NodeID id = p_nodes[i];
        TaskTypeDOD type = task_types[id];
        MemoryGrantPOD* grant = get_grant_mutable(id);
        if (!grant) continue;

        switch (type) {
            case TaskTypeDOD::QUERY_CULLER:
            case TaskTypeDOD::NATIVE_CPU: {
                TaskCPUMetadata& meta = cpu_metadata[id];
                if (meta.native_interface) {
                    INativeTask* task = static_cast<INativeTask*>(meta.native_interface);
                    TaskContextPOD ctx{ p_delta, grant };
                    task->execute(ctx);
                }
            } break;

            case TaskTypeDOD::COMPUTE_GPU: {
                if (compute_list == -1 && rd) compute_list = rd->compute_list_begin();
                
                const TaskGPUMetadata& meta = gpu_metadata[id];
                if (grant->uniform_set_handle != 0 && meta.pipeline_rid.is_valid()) {
                    godot::RID uniform_set;
                    *(reinterpret_cast<uint64_t*>(&uniform_set)) = grant->uniform_set_handle;

                    rd->compute_list_bind_compute_pipeline(compute_list, meta.pipeline_rid);
                    rd->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
                    rd->compute_list_dispatch(compute_list, meta.dispatch_x, meta.dispatch_y, meta.dispatch_z);
                }
            } break;

            case TaskTypeDOD::GODOT_REFLECTION: {
                TaskCPUMetadata& meta = cpu_metadata[id];
                if (meta.reflection_target) {
                    meta.reflection_target->call(meta.execution_method.c_str(), p_delta);
                }
            } break;
        }
    }

    if (compute_list != -1 && rd) {
        rd->compute_list_end();
    }
}

void TaskGraphDOD::_batch_resolve_wave(const NodeID* p_nodes, uint32_t p_count) {
    for (uint32_t i = 0; i < p_count; ++i) {
        NodeID id = p_nodes[i];
        const MemoryGrantPOD* grant = get_grant(id);
        if (!grant) continue;

        const auto& ports = output_port_map[id];
        if (ports.empty()) continue;

        if (port_constants[id].size() < ports.size()) {
            port_constants[id].resize(ports.size());
        }

        for (uint32_t port_idx = 0; port_idx < ports.size(); ++port_idx) {
            const TaskPortMetadata& meta = ports[port_idx];
            for (uint32_t p = 0; p < grant->part_count; ++p) {
                const GrantPartPOD& part = grant->parts[p];
                if (part.buffer_id == meta.buffer_id) {
                    void* src_addr = static_cast<uint8_t*>(part.raw_base_ptr);
                    port_constants[id][port_idx] = _raw_to_variant(src_addr, meta.type);
                    break;
                }
            }
        }
    }
}

void TaskGraphDOD::_variant_to_raw(const godot::Variant& p_var, void* p_dest, DataType p_type) {
    switch (p_type) {
        case DataType::BOOL:    *static_cast<uint32_t*>(p_dest) = p_var.operator bool() ? 1 : 0; break;
        case DataType::BYTE:    *static_cast<uint8_t*>(p_dest)  = (uint8_t)p_var.operator int(); break;
        case DataType::INT32:   *static_cast<int32_t*>(p_dest)  = p_var.operator int32_t(); break;
        case DataType::INT64:   *static_cast<int64_t*>(p_dest)  = p_var.operator int64_t(); break;
        case DataType::FLOAT32: *static_cast<float*>(p_dest)    = p_var.operator float(); break;
        case DataType::FLOAT64: *static_cast<double*>(p_dest)   = p_var.operator double(); break;
        case DataType::VECTOR2: { godot::Vector2 v = p_var; std::memcpy(p_dest, &v, 8); } break;
        case DataType::VECTOR3: { godot::Vector3 v = p_var; std::memcpy(p_dest, &v, 12); } break;
        case DataType::VECTOR4: { godot::Vector4 v = p_var; std::memcpy(p_dest, &v, 16); } break;
        case DataType::COLOR:   { godot::Color c = p_var; std::memcpy(p_dest, &c, 16); } break;
        default: break;
    }
}

godot::Variant TaskGraphDOD::_raw_to_variant(const void* p_src, DataType p_type) {
    switch (p_type) {
        case DataType::BOOL:    return godot::Variant(*static_cast<const uint32_t*>(p_src) != 0);
        case DataType::INT32:   return godot::Variant(*static_cast<const int32_t*>(p_src));
        case DataType::FLOAT32: return godot::Variant(*static_cast<const float*>(p_src));
        case DataType::VECTOR2: { godot::Vector2 v; std::memcpy(&v, p_src, 8); return v; }
        case DataType::VECTOR3: { godot::Vector3 v; std::memcpy(&v, p_src, 12); return v; }
        case DataType::COLOR:   { godot::Color c; std::memcpy(&c, p_src, 16); return c; }
        default: return godot::Variant();
    }
}

void TaskGraphDOD::_remap_ids(const std::vector<NodeID>& p_node_lut, const std::vector<EdgeID>& p_edge_lut) {
    MemoryGraphDOD::_remap_ids(p_node_lut, p_edge_lut);

    size_t new_size = build_nodes.size();
    std::vector<TaskTypeDOD> p_types(new_size);
    std::vector<TaskCPUMetadata> p_cpu(new_size);
    std::vector<TaskGPUMetadata> p_gpu(new_size);
    std::vector<std::vector<TaskPortMetadata>> p_in(new_size);
    std::vector<std::vector<TaskPortMetadata>> p_out(new_size);
    std::vector<std::vector<godot::Variant>> p_const(new_size);

    for (uint32_t i = 0; i < p_node_lut.size(); ++i) {
        NodeID n = p_node_lut[i];
        if (n != INVALID_ID) {
            p_types[n] = task_types[i];
            p_cpu[n] = std::move(cpu_metadata[i]);
            p_gpu[n] = std::move(gpu_metadata[i]);
            p_in[n] = std::move(input_port_map[i]);
            p_out[n] = std::move(output_port_map[i]);
            p_const[n] = std::move(port_constants[i]);
        }
    }

    task_types = std::move(p_types);
    cpu_metadata = std::move(p_cpu);
    gpu_metadata = std::move(p_gpu);
    input_port_map = std::move(p_in);
    output_port_map = std::move(p_out);
    port_constants = std::move(p_const);
    dirty_flags |= RESOURCES;
}

void TaskGraphDOD::defragment() { MemoryGraphDOD::defragment(); }
void TaskGraphDOD::clear() {
    task_types.clear(); cpu_metadata.clear(); gpu_metadata.clear();
    input_port_map.clear(); output_port_map.clear(); port_constants.clear();
    MemoryGraphDOD::clear();
}

} // namespace ideam::core