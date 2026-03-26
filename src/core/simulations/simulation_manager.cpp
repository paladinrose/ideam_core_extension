#include "simulation_manager.h"
#include "simulation_graph.h"
#include "simulation_task_metadata.h"
#include <algorithm>

namespace ideam::core {

SimulationManager::SimulationManager() {
    // Initialize the core memory manager upon construction.
    memory_manager = std::make_unique<MemoryManager>();
}

SimulationManager::~SimulationManager() {
    // Explicitly clear buckets. MemoryManager unique_ptr handles its own lifecycle.
    graph_bucket_process.clear();
    graph_bucket_physics.clear();
    graph_bucket_manual.clear();
    graph_bucket_interval.clear();
}

void SimulationManager::register_buffer(ISimulationBuffer* p_buffer) {
    if (p_buffer) {
        memory_manager->register_buffer(p_buffer->get_memory_buffer());
    }
}

void SimulationManager::unregister_buffer(ISimulationBuffer* p_buffer) {
    if (p_buffer) {
        memory_manager->unregister_buffer(p_buffer->get_memory_buffer());
    }
}

MemoryGrant* SimulationManager::request_grant(SimulationTaskMetadata* p_task) {
    if (!p_task) {
        return nullptr;
    }

    const std::vector<GrantPart>& requirements = p_task->requirements;
    
    return memory_manager->request_grant(requirements);
}

void SimulationManager::release_grant(MemoryGrant* p_grant) {
    if (p_grant) {
        memory_manager->release_grant(p_grant);
    }
}

void SimulationManager::register_managed_graph(SimulationGraph* p_graph, ExecutionTiming p_timing, double p_interval) {
    if (!p_graph) {
        return;
    }
    
    // Provide the graph with a reference to this orchestrator for grant acquisition.
    p_graph->set_manager(this);

    switch (p_timing) {
        case ExecutionTiming::PROCESS: 
            graph_bucket_process.push_back(p_graph); 
            break;
        case ExecutionTiming::PHYSICS_PROCESS: 
            graph_bucket_physics.push_back(p_graph); 
            break;
        case ExecutionTiming::MANUAL: 
            graph_bucket_manual.push_back(p_graph); 
            break;
        case ExecutionTiming::FIXED_INTERVAL: 
            graph_bucket_interval.push_back({p_graph, 0.0, p_interval}); 
            break;
    }
}

void SimulationManager::unregister_managed_graph(SimulationGraph* p_graph) {
    auto filter = [p_graph](SimulationGraph* g) { return g == p_graph; };
    
    graph_bucket_process.erase(
        std::remove_if(graph_bucket_process.begin(), graph_bucket_process.end(), filter), 
        graph_bucket_process.end()
    );
    
    graph_bucket_physics.erase(
        std::remove_if(graph_bucket_physics.begin(), graph_bucket_physics.end(), filter), 
        graph_bucket_physics.end()
    );
    
    graph_bucket_manual.erase(
        std::remove_if(graph_bucket_manual.begin(), graph_bucket_manual.end(), filter), 
        graph_bucket_manual.end()
    );
    
    graph_bucket_interval.erase(
        std::remove_if(graph_bucket_interval.begin(), graph_bucket_interval.end(), 
        [p_graph](const GraphEntry& e) { return e.graph == p_graph; }), 
        graph_bucket_interval.end()
    );
}

void SimulationManager::execute_process(double p_delta) {
    // Mirroring Godot's _process pump.
    for (auto graph : graph_bucket_process) {
        graph->execute_frame(p_delta);
    }
}

void SimulationManager::execute_physics(double p_delta) {
    // Mirroring Godot's _physics_process pump.
    for (auto graph : graph_bucket_physics) {
        graph->execute_frame(p_delta);
    }
}

void SimulationManager::execute_manual(double p_delta) {
    // Entry point for user-driven or triggered simulation steps.
    for (auto graph : graph_bucket_manual) {
        graph->execute_frame(p_delta);
    }
}

void SimulationManager::execute_interval(double p_delta) {
    // Logic for graphs that run on a specialized sub-ticker.
    for (auto& entry : graph_bucket_interval) {
        entry.timer += p_delta;
        if (entry.timer >= entry.interval) {
            entry.graph->execute_frame(entry.timer);
            entry.timer = 0.0;
        }
    }
}

} // namespace ideam::core