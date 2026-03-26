#include "simulation_manager_dod.h"
#include "simulation_graph_dod.h"
#include <algorithm>

namespace ideam::core {

SimulationManagerDOD::~SimulationManagerDOD() {
    graph_bucket_process.clear();
    graph_bucket_physics.clear();
    graph_bucket_manual.clear();
    graph_bucket_interval.clear();
}

void SimulationManagerDOD::register_graph(SimulationGraphDOD* p_graph, ExecutionTiming p_timing, double p_interval) {
    if (!p_graph) {
        return;
    }

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

void SimulationManagerDOD::unregister_graph(SimulationGraphDOD* p_graph) {
    auto filter = [p_graph](SimulationGraphDOD* g) { return g == p_graph; };
    
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

void SimulationManagerDOD::execute_process(double p_delta) {
    for (auto graph : graph_bucket_process) {
        graph->execute_frame(p_delta);
    }
}

void SimulationManagerDOD::execute_physics(double p_delta) {
    for (auto graph : graph_bucket_physics) {
        graph->execute_frame(p_delta);
    }
}

void SimulationManagerDOD::execute_manual(double p_delta) {
    for (auto graph : graph_bucket_manual) {
        graph->execute_frame(p_delta);
    }
}

void SimulationManagerDOD::execute_interval(double p_delta) {
    for (auto& entry : graph_bucket_interval) {
        entry.timer += p_delta;
        if (entry.timer >= entry.interval) {
            // Note: We pass the accumulated timer to maintain temporal consistency
            entry.graph->execute_frame(entry.timer);
            entry.timer = 0.0;
        }
    }
}

void SimulationManagerDOD::flush_gpu() {
    if (memory_manager) {
        memory_manager->flush_gpu_updates();
    }
}

} // namespace ideam::core