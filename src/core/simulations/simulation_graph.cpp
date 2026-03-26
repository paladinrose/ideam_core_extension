#include "simulation_graph.h"
#include "../simulation_manager.h"

namespace ideam::core {

NodeID SimulationGraph::add_simulation_task(TaskType p_type, const std::vector<SimulationTaskMetadata::BufferAccess>& p_reqs) {
    // 1. Create the base task node
    NodeID id = add_task_node(p_type);
    
    // 2. Initialize simulation metadata
    SimulationTaskMetadata meta;
    meta.type = p_type;
    meta.requirements = p_reqs;
    
    sim_metadata[id] = std::move(meta);

    // 3. Mark requirements as dirty to trigger a bake on the next validation
    dirty_flags |= RESOURCES;
    
    return id;
}

void SimulationGraph::execute_frame(double p_delta) {
    // We now leverage the stabilized TaskGraph execution engine.
    // This handles Kahn waves, validate_grants (with our GPU logic), 
    // and Godot RenderingDevice synchronization.
    execute_graph(p_delta);
}

void SimulationGraph::_bake_requirements() {
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id == INVALID_ID) continue;

        auto it = sim_metadata.find(i);
        if (it == sim_metadata.end()) continue;

        std::vector<GrantPart> node_reqs;
        node_reqs.reserve(it->second.requirements.size());

        for (const auto& access : it->second.requirements) {
            GrantPart part;
            part.buffer_id = access.buffer_id;
            part.access_mode = access.mode;
            part.selection = access.selection; // Direct pointer transfer
            node_reqs.push_back(part);
        }

        set_node_requirements(i, node_reqs);
    }
    dirty_flags &= ~RESOURCES;
}

SimulationTaskMetadata* SimulationGraph::get_sim_metadata(NodeID p_id) {
    auto it = sim_metadata.find(p_id);
    if (it != sim_metadata.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace ideam::core