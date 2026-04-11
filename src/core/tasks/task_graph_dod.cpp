#include "task_graph_dod.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>
#include <array>

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
        
        input_port_meta.resize(id + 1);
        output_port_meta.resize(id + 1);
        constant_port_meta.resize(id + 1);
        baked_connections.resize(id + 1);
        transient_bytes_meta.resize(id + 1, 0);
    }

    task_types[id] = p_type;
    return id;
}

void TaskGraphDOD::configure_cpu_task(NodeID p_id, godot::Object* p_target, const godot::StringName& p_method) {
    if (p_id < cpu_metadata.size()) {
        cpu_metadata[p_id].reflection_target = p_target;
        cpu_metadata[p_id].execution_method = p_method;
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

void TaskGraphDOD::configure_native_interface(NodeID p_id, std::unique_ptr<INativeTask> p_interface) {
    if (p_id < cpu_metadata.size() && p_interface) {
        cpu_metadata[p_id].native_interface = p_interface.get();
        owned_native_tasks.push_back(std::move(p_interface));
    }
}

void TaskGraphDOD::set_node_transient_requirement(NodeID p_id, uint32_t p_bytes) {
    if (p_id < transient_bytes_meta.size()) {
        transient_bytes_meta[p_id] = p_bytes;
        dirty_flags |= STRUCTURE; 
    }
}

void TaskGraphDOD::set_port_mappings(NodeID p_id, bool p_input, std::span<const TaskPortMetadata> p_mappings) {
    if (p_id >= build_nodes.size() || build_nodes.id[p_id] == INVALID_ID) return;

    auto& meta_array = p_input ? input_port_meta : output_port_meta;
    auto& data_array = p_input ? input_port_data : output_port_data;

    if (p_id >= meta_array.size()) meta_array.resize(build_nodes.size());

    meta_array[p_id].offset = static_cast<uint32_t>(data_array.size());
    meta_array[p_id].count = static_cast<uint32_t>(p_mappings.size());
    
    data_array.insert(data_array.end(), p_mappings.begin(), p_mappings.end());
    dirty_flags |= RESOURCES;
}

void TaskGraphDOD::set_port_constants(NodeID p_id, std::span<const godot::Variant> p_constants) {
    if (p_id >= build_nodes.size() || build_nodes.id[p_id] == INVALID_ID) return;

    if (p_id >= constant_port_meta.size()) constant_port_meta.resize(build_nodes.size());

    constant_port_meta[p_id].offset = static_cast<uint32_t>(constant_port_data.size());
    constant_port_meta[p_id].count = static_cast<uint32_t>(p_constants.size());
    
    constant_port_data.insert(constant_port_data.end(), p_constants.begin(), p_constants.end());
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

    validate_grants();

    if (dirty_flags & RESOURCES) {
        _bake_port_connections();
        dirty_flags &= ~RESOURCES;
    }

    manager->flush_gpu_updates();

    // Ensure Utility Command Buffers exist (4MB capacity each)
    // Tier 1 is a raw byte arena, so stride is 1
    _ensure_buffer(tier1_buffer_id, 4194304, 1, 64); 
    // Tier 2 is a queue of int64_t indices
    _ensure_buffer(tier2_buffer_id, 4194304, sizeof(int64_t), 64);

    MemoryGrantPOD global_grant;
    std::array<GrantPartPOD, 4> reqs = {{
        {.buffer_id = wave_node_id, .element_stride = sizeof(NodeID), .access_mode = BufferAccessMode::READ},
        {.buffer_id = wave_meta_id, .element_stride = sizeof(WaveInfo), .access_mode = BufferAccessMode::READ},
        {.buffer_id = tier1_buffer_id, .element_stride = 1, .access_mode = BufferAccessMode::WRITE}, // 1 byte raw bump arena
        {.buffer_id = tier2_buffer_id, .element_stride = sizeof(int64_t), .access_mode = BufferAccessMode::WRITE}
    }};

    for (auto& req : reqs) {
        if (const MemoryBufferPOD* b = manager->get_buffer(req.buffer_id)) {
            req.selection.mode = SelectionMode::RANGE;
            req.selection.start_index = 0;
            req.selection.element_count = b->max_elements;
            req.selection.capacity = b->max_elements;
        }
    }

    if (!manager->bake_grant(global_grant, reqs)) return;
    
    auto n_view = _get_paged_view<NodeID, FlatStrategy>(global_grant, 0);
    auto m_view = _get_paged_view<WaveInfo, FlatStrategy>(global_grant, 1);
    
    const MemoryBufferPOD* t1_buf = manager->get_buffer(global_grant.parts[2].buffer_id);
    const MemoryBufferPOD* t2_buf = manager->get_buffer(global_grant.parts[3].buffer_id);
    const size_t t1_cap = t1_buf ? t1_buf->capacity_bytes : 0;
    const size_t t2_cap = t2_buf ? t2_buf->capacity_bytes : 0;

    uint8_t* tier1_arena = global_grant.parts[2].raw_base_ptr;
    size_t t1_chunk = t1_cap / (build_nodes.size() > 0 ? build_nodes.size() : 1);
    
    tier1_meta.resize(build_nodes.size());
    for(size_t i = 0; i < build_nodes.size(); ++i) {
        tier1_meta[i].arena_ptr = tier1_arena + (i * t1_chunk);
        tier1_meta[i].capacity = static_cast<uint32_t>(t1_chunk);
        tier1_meta[i].reset();
    }

    const MemoryBufferPOD* m_buf = manager->get_buffer(wave_meta_id);
    uint32_t wave_count = (m_buf) ? static_cast<uint32_t>(m_buf->capacity_bytes / sizeof(WaveInfo)) : 0;

    std::vector<NodeID> current_wave_nodes;

    for (uint32_t w = 0; w < wave_count; ++w) {
        WaveInfo wave = m_view[w];
        if (wave.count == 0) break; 

        current_wave_nodes.resize(wave.count);
        for (uint32_t i = 0; i < wave.count; ++i) {
            current_wave_nodes[i] = n_view[wave.offset + i];
        }

        // --- Transient Allocation Pass ---
        manager->reset_transient();
        std::vector<void*> wave_workspaces(wave.count, nullptr);
        for (uint32_t i = 0; i < wave.count; ++i) {
            size_t req = transient_bytes_meta[current_wave_nodes[i]];
            if (req > 0) {
                wave_workspaces[i] = manager->allocate_transient(req, 64);
            }
        }

        int64_t* tier2_arena = reinterpret_cast<int64_t*>(global_grant.parts[3].raw_base_ptr);
        size_t t2_chunk = (t2_cap / sizeof(int64_t)) / wave.count;
        
        tier2_meta.resize(wave.count);
        for(uint32_t i = 0; i < wave.count; ++i) {
            tier2_meta[i].queued_indices = tier2_arena + (i * t2_chunk);
            tier2_meta[i].capacity = static_cast<int64_t>(t2_chunk);
            tier2_meta[i].reset();
        }

        for (uint32_t i = 0; i < wave.count; ++i) {
            _clean_selections(current_wave_nodes[i], wave_workspaces[i]);
        }

        _batch_setup_wave(current_wave_nodes.data(), wave.count);
        _batch_execute_wave(current_wave_nodes.data(), wave.count, p_delta, wave_workspaces.data());
        
        _process_tier2_commands(current_wave_nodes.data(), wave.count);
        _batch_resolve_wave(current_wave_nodes.data(), wave.count);
    }

    _process_tier1_commands();

    manager->release_grant(global_grant);
}

void TaskGraphDOD::_clean_selections(NodeID p_id, void* p_workspace) {
    SelectionMetadata* meta = get_selection_meta(p_id);
    if (!meta || meta->dirty_parts_mask == 0) return;

    MemoryGrantPOD* grant = get_grant_mutable(p_id);
    if (!grant) return;

    TaskTypeDOD type = task_types[p_id];
    
    if (type == TaskTypeDOD::NATIVE_CPU || type == TaskTypeDOD::QUERY_CULLER) {
        TaskCPUMetadata& cpu_meta = cpu_metadata[p_id];
        if (cpu_meta.native_interface) {
            TaskContextPOD ctx{ 0.0, grant, manager, nullptr, nullptr, p_workspace }; 
            cpu_meta.native_interface->cull_selections(ctx, meta->dirty_parts_mask);
        }
    }

    meta->dirty_parts_mask = 0;
}

void TaskGraphDOD::_bake_port_connections() {
    for (auto& vec : baked_connections) vec.clear();

    for (const auto& edge : build_edges) {
        if (edge.id == INVALID_ID) continue;

        const MemoryGrantPOD* src_grant = get_grant(edge.from_node);
        const MemoryGrantPOD* dst_grant = get_grant(edge.to_node);
        if (!src_grant || !dst_grant) continue;

        if (edge.from_node >= output_port_meta.size() || edge.to_node >= input_port_meta.size()) continue;

        TaskPortOffsets out_offsets = output_port_meta[edge.from_node];
        TaskPortOffsets in_offsets = input_port_meta[edge.to_node];

        if (edge.from_port >= out_offsets.count || edge.to_port >= in_offsets.count) continue;

        TaskPortMetadata& out_meta = output_port_data[out_offsets.offset + edge.from_port];
        TaskPortMetadata& in_meta = input_port_data[in_offsets.offset + edge.to_port];

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
            if (edge.to_node >= baked_connections.size()) baked_connections.resize(edge.to_node + 1);
            baked_connections[edge.to_node].push_back(conn);
        }
    }
}

void TaskGraphDOD::_batch_setup_wave(const NodeID* p_nodes, uint32_t p_count) {
    for (uint32_t i = 0; i < p_count; ++i) {
        NodeID id = p_nodes[i];
        
        if (id < baked_connections.size()) {
            for (const auto& conn : baked_connections[id]) {
                std::memcpy(conn.dst_ptr, conn.src_ptr, conn.byte_size);
            }
        }

        const MemoryGrantPOD* grant = get_grant(id);
        if (!grant) continue;

        if (id >= input_port_meta.size() || id >= constant_port_meta.size()) continue;

        TaskPortOffsets in_meta = input_port_meta[id];
        TaskPortOffsets const_meta = constant_port_meta[id];

        for (uint32_t p_idx = 0; p_idx < in_meta.count; ++p_idx) {
            if (p_idx >= const_meta.count) break;

            const godot::Variant& var = constant_port_data[const_meta.offset + p_idx];
            if (var.get_type() == godot::Variant::NIL) continue;
            
            const TaskPortMetadata& port = input_port_data[in_meta.offset + p_idx];

            for (uint32_t g_part = 0; g_part < grant->part_count; ++g_part) {
                if (grant->parts[g_part].buffer_id == port.buffer_id) {
                    _variant_to_raw(var, grant->parts[g_part].raw_base_ptr, port.type);
                    break;
                }
            }
        }
    }
}

void TaskGraphDOD::_batch_execute_wave(const NodeID* p_nodes, uint32_t p_count, double p_delta, void** p_workspaces) {
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
                    TaskContextPOD ctx{ p_delta, grant, manager, &tier1_meta[id], &tier2_meta[i], p_workspaces[i] };
                    meta.native_interface->execute(ctx);
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
                if (meta.reflection_target && meta.execution_method != godot::StringName()) {
                    meta.reflection_target->call(meta.execution_method, p_delta);
                }
            } break;
        }
    }

    if (compute_list != -1 && rd) {
        rd->compute_list_end();
    }
}

void TaskGraphDOD::_process_tier2_commands(const NodeID* p_nodes, uint32_t p_count) {
    for (uint32_t i = 0; i < p_count; ++i) {
        TaskSelectionCommandPOD& cmd = tier2_meta[i];
        if (cmd.count == 0 || cmd.target_buffer_id == 0) continue;

        MemoryGrantPOD* grant = get_grant_mutable(p_nodes[i]);
        if (!grant) continue;

        for (uint32_t p = 0; p < grant->part_count; ++p) {
            if (grant->parts[p].buffer_id == cmd.target_buffer_id) {
                MemoryBufferSelectionPOD& sel = grant->parts[p].selection;
                
                for (int64_t k = 0; k < cmd.count; ++k) {
                    const int64_t idx = cmd.queued_indices[k];
                    if (sel.mode == SelectionMode::DENSE) {
                        sel.data.bitset[idx >> 6] |= (1ULL << (idx & 63));
                    } else if (sel.mode == SelectionMode::SPARSE) {
                        if (sel.element_count < sel.capacity) {
                            sel.data.indices[sel.element_count++] = idx;
                        }
                    }
                }
                break;
            }
        }
        cmd.reset();
    }
}

void TaskGraphDOD::_process_tier1_commands() {
    for (auto& meta : tier1_meta) {
        meta.reset(); 
    }
}

void TaskGraphDOD::_batch_resolve_wave(const NodeID* p_nodes, uint32_t p_count) {
    for (uint32_t i = 0; i < p_count; ++i) {
        NodeID id = p_nodes[i];
        const MemoryGrantPOD* grant = get_grant(id);
        if (!grant || id >= output_port_meta.size()) continue;

        TaskPortOffsets out_meta = output_port_meta[id];
        if (out_meta.count == 0) continue;

        if (id >= constant_port_meta.size()) constant_port_meta.resize(build_nodes.size());

        if (constant_port_meta[id].count < out_meta.count) {
            constant_port_meta[id].offset = static_cast<uint32_t>(constant_port_data.size());
            constant_port_meta[id].count = out_meta.count;
            constant_port_data.resize(constant_port_data.size() + out_meta.count);
        }

        for (uint32_t port_idx = 0; port_idx < out_meta.count; ++port_idx) {
            const TaskPortMetadata& meta = output_port_data[out_meta.offset + port_idx];
            
            for (uint32_t p = 0; p < grant->part_count; ++p) {
                const GrantPartPOD& part = grant->parts[p];
                if (part.buffer_id == meta.buffer_id) {
                    void* src_addr = static_cast<uint8_t*>(part.raw_base_ptr);
                    constant_port_data[constant_port_meta[id].offset + port_idx] = _raw_to_variant(src_addr, meta.type);
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
    
    std::vector<TaskPortOffsets> p_in_meta(new_size);
    std::vector<TaskPortMetadata> p_in_data;
    p_in_data.reserve(input_port_data.size());

    std::vector<TaskPortOffsets> p_out_meta(new_size);
    std::vector<TaskPortMetadata> p_out_data;
    p_out_data.reserve(output_port_data.size());

    std::vector<TaskPortOffsets> p_const_meta(new_size);
    std::vector<godot::Variant> p_const_data;
    p_const_data.reserve(constant_port_data.size());

    std::vector<uint32_t> p_transient(new_size, 0);

    for (uint32_t i = 0; i < p_node_lut.size(); ++i) {
        NodeID n = p_node_lut[i];
        if (n != INVALID_ID) {
            p_types[n] = task_types[i];
            p_cpu[n] = std::move(cpu_metadata[i]);
            p_gpu[n] = std::move(gpu_metadata[i]);
            p_transient[n] = transient_bytes_meta[i];
            
            if (i < input_port_meta.size() && input_port_meta[i].count > 0) {
                p_in_meta[n].offset = static_cast<uint32_t>(p_in_data.size());
                p_in_meta[n].count = input_port_meta[i].count;
                auto it = input_port_data.begin() + input_port_meta[i].offset;
                p_in_data.insert(p_in_data.end(), it, it + input_port_meta[i].count);
            }
            
            if (i < output_port_meta.size() && output_port_meta[i].count > 0) {
                p_out_meta[n].offset = static_cast<uint32_t>(p_out_data.size());
                p_out_meta[n].count = output_port_meta[i].count;
                auto it = output_port_data.begin() + output_port_meta[i].offset;
                p_out_data.insert(p_out_data.end(), it, it + output_port_meta[i].count);
            }
            
            if (i < constant_port_meta.size() && constant_port_meta[i].count > 0) {
                p_const_meta[n].offset = static_cast<uint32_t>(p_const_data.size());
                p_const_meta[n].count = constant_port_meta[i].count;
                auto it = constant_port_data.begin() + constant_port_meta[i].offset;
                p_const_data.insert(p_const_data.end(), it, it + constant_port_meta[i].count);
            }
        }
    }

    task_types = std::move(p_types);
    cpu_metadata = std::move(p_cpu);
    gpu_metadata = std::move(p_gpu);
    transient_bytes_meta = std::move(p_transient);
    
    input_port_meta = std::move(p_in_meta);
    input_port_data = std::move(p_in_data);
    output_port_meta = std::move(p_out_meta);
    output_port_data = std::move(p_out_data);
    constant_port_meta = std::move(p_const_meta);
    constant_port_data = std::move(p_const_data);
    
    baked_connections.assign(new_size, std::vector<TaskPortConnectionDOD>());
}

void TaskGraphDOD::defragment() { MemoryGraphDOD::defragment(); }
void TaskGraphDOD::clear() {
    task_types.clear(); cpu_metadata.clear(); gpu_metadata.clear();
    input_port_meta.clear(); input_port_data.clear();
    output_port_meta.clear(); output_port_data.clear();
    constant_port_meta.clear(); constant_port_data.clear();
    transient_bytes_meta.clear();
    baked_connections.clear();
    owned_native_tasks.clear(); // Important: Cleans up instantiated tasks
    MemoryGraphDOD::clear();
}

} // namespace ideam::core