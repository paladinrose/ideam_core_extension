#ifndef IDEAM_GODOT_SIMULATION_ENGINE_H
#define IDEAM_GODOT_SIMULATION_ENGINE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "../../core/simulations/simulation_manager.h"
#include "simulation_buffer_resource.h"
#include "simulation_graph_resource.h"

#include <unordered_set>
#include <memory>

namespace godot {

class SimulationEngine : public Node {
    GDCLASS(SimulationEngine, Node);

private:
    // Core high-performance orchestrator
    std::unique_ptr<ideam::core::SimulationManager> manager;

    // Configuration Properties
    bool active = true;
    TypedArray<SimulationBufferResource> buffers;
    TypedArray<SimulationGraphResource> graphs;

    // Internal Pinning Sets to prevent garbage collection of registered resources
    std::unordered_set<Ref<SimulationBufferResource>> active_buffers;
    std::unordered_set<Ref<SimulationGraphResource>> active_graphs;

    // Internal Helpers
    void _sync_with_core();
    void _clear_core_bindings();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SimulationEngine();
    ~SimulationEngine();

    // --- Property Accessors ---
    void set_active(bool p_active);
    bool is_active() const;

    void set_buffers(TypedArray<SimulationBufferResource> p_buffers);
    TypedArray<SimulationBufferResource> get_buffers() const;

    void set_graphs(TypedArray<SimulationGraphResource> p_graphs);
    TypedArray<SimulationGraphResource> get_graphs() const;

    // --- Manual Control ---
    void execute_manual(double p_delta);
    uint32_t get_version() const;
    
    // --- Lifecycle ---
    void setup_simulation();
};

} // namespace godot

#endif // IDEAM_GODOT_SIMULATION_ENGINE_H