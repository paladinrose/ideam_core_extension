#ifndef IDEAM_CORE_TASK_GRAPH_CPP
#define IDEAM_CORE_TASK_GRAPH_CPP

#include "task_graph.h"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::core {

TaskGraph::TaskGraph() {
    auto rs = godot::RenderingServer::get_singleton();
    if (rs) {
        rd = rs->get_rendering_device();
    }
}

TaskGraph::~TaskGraph() {
    clear();
}

NodeID TaskGraph::add_task_node(TaskType p_type) {
    NodeID new_id = IdeamGraph::add_node(0);
    if (new_id >= task_registry.size()) {
        task_registry.resize(nodes.size());
    }
    task_registry[new_id] = TaskMetadata();
    task_registry[new_id].type = p_type;
    return new_id;
}

/**
 * validate_grants (Override)
 * Specialized audit that injects GPU hints into the MemoryManager request
 * based on the TaskType of each node.
 */
void TaskGraph::validate_grants() {
    if (!manager) return;

    // 1. Structural/Resource Bake (Inherited logic trigger)
    // Note: dirty_flags is assumed protected in MemoryGraph
    if (dirty_flags & (RESOURCES | STRUCTURE)) {
        _bake_requirements();
    }

    // 2. Lease Audit
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id == INVALID_ID) continue;

        MemoryGrant* current_lease = nullptr;
        if (persistent_grants.count(i)) {
            current_lease = persistent_grants[i];
        }

        // Determine if this specific node requires GPU backing
        bool needs_gpu = (task_registry[i].type == TaskType::COMPUTE_GPU);

        // Re-acquisition triggers if:
        // - Lease is missing
        // - Lease is invalid (Manager rebased memory or Selection version bumped)
        // - GPU state mismatch (If a lease exists but lacks a UniformSet when we need one)
        bool gpu_mismatch = (needs_gpu && current_lease && !current_lease->get_uniform_set_rid().is_valid());

        if (!current_lease || !current_lease->is_valid() || gpu_mismatch) {
            
            if (current_lease) {
                manager->release_grant(current_lease);
                persistent_grants.erase(i);
            }

            uint32_t count = 0;
            const GrantPart* parts_ptr = get_node_requirements(i, count);
            
            if (count > 0) {
                // Request new lease from authority with the task-specific GPU hint
                std::vector<GrantPart> reqs(parts_ptr, parts_ptr + count);
                MemoryGrant* new_grant = manager->request_grant(reqs, needs_gpu);
                
                if (new_grant) {
                    persistent_grants[i] = new_grant;
                }
            }
        }
    }
}

void TaskGraph::execute_graph(double p_delta) {
    validate_grants(); // Ensure all leases are valid before execution

    const auto& waves = get_execution_waves();
    
    for (const auto& wave : waves) {
        int64_t compute_list = -1;

        // 2. Wave Dispatch
        for (NodeID id : wave) {
            TaskMetadata& task = task_registry[id];
            
            if (task.type == TaskType::COMPUTE_GPU) {
                if (compute_list == -1 && rd) {
                    compute_list = rd->compute_list_begin();
                }
                _dispatch_gpu_task(id, compute_list);
            } else {
                // Synchronous CPU execution (Native or Reflection)
                _execute_node_logic(id);
            }
        }

        // 3. Close Wave-specific GPU work and synchronize
        if (compute_list != -1 && rd) {
            rd->compute_list_end();
            // Inject barrier to protect next wave's potential dependencies (Read-After-Write)
            rd->barrier(godot::RenderingDevice::BARRIER_MASK_COMPUTE);
        }
    }
}

void TaskGraph::_execute_node_logic(NodeID p_id, godot::Object* p_executor) {
    TaskMetadata& task = task_registry[p_id];
    
    if (task.type == TaskType::NATIVE_CPU && task.native_interface) {
        task.native_interface->execute_native(get_grant(p_id));
        task.has_executed = true;
    } 
    else if (task.type == TaskType::GODOT_REFLECTION && p_executor) {
        task_phase_setup(p_id, p_executor);
        task_phase_execute(p_id, p_executor);
        task_phase_resolve(p_id, p_executor);
    }
}

void TaskGraph::_dispatch_gpu_task(NodeID p_id, int64_t p_compute_list) {
    if (!rd || p_compute_list == -1) return;

    TaskMetadata& task = task_registry[p_id];
    MemoryGrant* lease = get_grant(p_id);

    // Verify lease validity and GPU resource availability
    if (!lease || !lease->is_valid() || !task.pipeline_rid.is_valid()) return;

    godot::RID uniform_set = lease->get_uniform_set_rid();
    if (!uniform_set.is_valid()) return;

    rd->compute_list_bind_compute_pipeline(p_compute_list, task.pipeline_rid);
    rd->compute_list_bind_uniform_set(p_compute_list, uniform_set, 0);
    rd->compute_list_dispatch(p_compute_list, task.dispatch_x, task.dispatch_y, task.dispatch_z);
    
    task.has_executed = true;
}

TaskMetadata* TaskGraph::get_task(NodeID p_id) {
    if (p_id < task_registry.size() && nodes[p_id].id != INVALID_ID) {
        return &task_registry[p_id];
    }
    return nullptr;
}

void TaskGraph::set_task_input(NodeID p_id, uint32_t p_port_id, const godot::Variant& p_value) {
    if (TaskMetadata* task = get_task(p_id)) {
        task->input_values[p_port_id] = p_value;
    }
}

godot::Variant TaskGraph::get_task_output(NodeID p_id, uint32_t p_port_id) {
    if (TaskMetadata* task = get_task(p_id)) {
        return task->output_values.get(p_port_id, godot::Variant());
    }
    return godot::Variant();
}

void TaskGraph::task_phase_setup(NodeID p_id, godot::Object* p_executor) {
    TaskMetadata* task = get_task(p_id);
    if (!task || !p_executor) return;

    for (const auto& mapping : task->input_mappings) {
        godot::Variant val = task->input_values.get(mapping.port_id, godot::Variant());
        if (p_executor->has_method(mapping.target_name.c_str())) {
            p_executor->call(mapping.target_name.c_str(), val);
        } else {
            p_executor->set(mapping.target_name.c_str(), val);
        }
    }
}

void TaskGraph::task_phase_execute(NodeID p_id, godot::Object* p_executor) {
    TaskMetadata* task = get_task(p_id);
    if (task && !task->execution_method.empty() && p_executor) {
        task->last_return_value = p_executor->call(task->execution_method.c_str());
    }
}

void TaskGraph::task_phase_resolve(NodeID p_id, godot::Object* p_executor) {
    TaskMetadata* task = get_task(p_id);
    if (!task || !p_executor) return;

    task->output_values.clear();
    for (const auto& mapping : task->output_mappings) {
        if (mapping.is_return_value) {
            task->output_values[mapping.port_id] = task->last_return_value;
        } else if (p_executor->has_method(mapping.target_name.c_str())) {
            task->output_values[mapping.port_id] = p_executor->call(mapping.target_name.c_str());
        } else {
            task->output_values[mapping.port_id] = p_executor->get(mapping.target_name.c_str());
        }
    }
}

void TaskGraph::remove_node(NodeID p_id) {
    if (p_id < task_registry.size()) {
        task_registry[p_id] = TaskMetadata();
    }
    MemoryGraph::remove_node(p_id);
}

void TaskGraph::defragment() {
    if (nodes.empty()) {
        clear();
        return;
    }

    std::vector<NodeID> node_lut(nodes.size(), INVALID_ID);
    uint32_t next_valid_idx = 0;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id != INVALID_ID) {
            node_lut[i] = next_valid_idx++;
        }
    }

    std::vector<TaskMetadata> new_tasks(next_valid_idx);
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (node_lut[i] != INVALID_ID) {
            new_tasks[node_lut[i]] = std::move(task_registry[i]);
        }
    }
    task_registry = std::move(new_tasks);

    MemoryGraph::defragment();
}

void TaskGraph::on_topology_changed() {
    MemoryGraph::on_topology_changed();
}

void TaskGraph::clear() {
    task_registry.clear();
    MemoryGraph::clear();
}

} // namespace ideam::core

#endif // IDEAM_CORE_TASK_GRAPH_CPP