#ifndef IDEAM_CORE_SIMULATION_MANAGER_DOD_H
#define IDEAM_CORE_SIMULATION_MANAGER_DOD_H

#include "../memory/memory_manager_dod.h"
#include <vector>
#include <cstdint>

namespace ideam::core {

class SimulationGraphDOD;

/**
 * ExecutionTiming
 * Mirrors Godot's execution methodology for bucketed updates.
 */
enum class ExecutionTiming : int64_t {
    MANUAL = 0,
    PROCESS = 1,
    PHYSICS_PROCESS = 2,
    FIXED_INTERVAL = 3
};

/**
 * SimulationManagerDOD
 * The high-performance orchestrator for the Ideam simulation engine.
 * * DESIGN NOTES:
 * 1. Agnostic of Godot types; operates on raw pointers and buckets.
 * 2. Does NOT own the MemoryManagerDOD; it holds a volatile pointer to one.
 * 3. Chiefly responsible for pumping SimulationGraphDODs.
 */
class SimulationManagerDOD {
private:
    // Non-owning pointer to the memory authority.
    MemoryManagerDOD* memory_manager = nullptr;

    // --- Execution Buckets ---
    struct GraphEntry { 
        SimulationGraphDOD* graph; 
        double timer = 0.0; 
        double interval = 0.1; 
    };

    std::vector<SimulationGraphDOD*> graph_bucket_process;
    std::vector<SimulationGraphDOD*> graph_bucket_physics;
    std::vector<SimulationGraphDOD*> graph_bucket_manual;
    std::vector<GraphEntry> graph_bucket_interval;

public:
    SimulationManagerDOD() = default;
    ~SimulationManagerDOD();

    // --- Configuration ---
    void set_memory_manager(MemoryManagerDOD* p_memory_manager) { memory_manager = p_memory_manager; }
    [[nodiscard]] MemoryManagerDOD* get_memory_manager() const { return memory_manager; }

    // --- Graph Lifecycle ---
    /**
     * register_graph
     * Places a graph into an execution bucket.
     */
    void register_graph(SimulationGraphDOD* p_graph, ExecutionTiming p_timing, double p_interval = 0.1);
    
    /**
     * unregister_graph
     * Removes a graph from all execution buckets.
     */
    void unregister_graph(SimulationGraphDOD* p_graph);
    
    // --- Execution Entry Points ---
    // These are pumped by the Godot-facing 'Simulator' Node.
    void execute_process(double p_delta);
    void execute_physics(double p_delta);
    void execute_manual(double p_delta);
    void execute_interval(double p_delta);

    // --- System Utility ---
    /**
     * flush_gpu
     * Triggers the MemoryManager to push CPU-dirty buffers to VRAM.
     */
    void flush_gpu();
};

} // namespace ideam::core

#endif // IDEAM_CORE_SIMULATION_MANAGER_DOD_H