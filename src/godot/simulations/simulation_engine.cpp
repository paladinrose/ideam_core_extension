#include "simulation_engine.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void SimulationEngine::_bind_methods() {
    // Active Toggle
    ClassDB::bind_method(D_METHOD("set_active", "active"), &SimulationEngine::set_active);
    ClassDB::bind_method(D_METHOD("is_active"), &SimulationEngine::is_active);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

    // Buffer Array
    ClassDB::bind_method(D_METHOD("set_buffers", "buffers"), &SimulationEngine::set_buffers);
    ClassDB::bind_method(D_METHOD("get_buffers"), &SimulationEngine::get_buffers);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "buffers", PROPERTY_HINT_ARRAY_TYPE, "SimulationBufferResource"), "set_buffers", "get_buffers");

    // Graph Array
    ClassDB::bind_method(D_METHOD("set_graphs", "graphs"), &SimulationEngine::set_graphs);
    ClassDB::bind_method(D_METHOD("get_graphs"), &SimulationEngine::get_graphs);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "graphs", PROPERTY_HINT_ARRAY_TYPE, "SimulationGraphResource"), "set_graphs", "get_graphs");

    // Manual Operations
    ClassDB::bind_method(D_METHOD("setup_simulation"), &SimulationEngine::setup_simulation);
    ClassDB::bind_method(D_METHOD("execute_manual", "delta"), &SimulationEngine::execute_manual);
    ClassDB::bind_method(D_METHOD("get_version"), &SimulationEngine::get_version);
}

SimulationEngine::SimulationEngine() {
    manager = std::make_unique<ideam::core::SimulationManager>();
    
    // We use internal process notifications to drive the core buckets
    set_process_internal(true);
    set_physics_process_internal(true);
}

SimulationEngine::~SimulationEngine() {
    _clear_core_bindings();
}

void SimulationEngine::_notification(int p_what) {
    if (!active || !manager) {
        return;
    }

    switch (p_what) {
        case NOTIFICATION_READY: {
            // Automatic setup on ready if resources are present
            setup_simulation();
        } break;

        case NOTIFICATION_INTERNAL_PROCESS: {
            double delta = get_process_delta_time();
            manager->execute_process(delta);
            manager->execute_interval(delta);
        } break;

        case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
            double delta = get_physics_process_delta_time();
            manager->execute_physics(delta);
        } break;
        
        case NOTIFICATION_PREDELETE: {
            _clear_core_bindings();
        } break;
    }
}

void SimulationEngine::setup_simulation() {
    _clear_core_bindings();
    _sync_with_core();
    UtilityFunctions::print("[SimulationEngine] Core re-initialized. Version: ", get_version());
}

void SimulationEngine::_sync_with_core() {
    // 1. Register Buffers first (as Graphs depend on them)
    for (int i = 0; i < buffers.size(); ++i) {
        Ref<SimulationBufferResource> res = buffers[i];
        if (res.is_valid()) {
            auto raw_buf = res->get_simulation_buffer_ptr();
            if (raw_buf) {
                manager->register_buffer(raw_buf);
                active_buffers.insert(res); // Pinning
            }
        }
    }

    // 2. Register Graphs
    for (int i = 0; i < graphs.size(); ++i) {
        Ref<SimulationGraphResource> res = graphs[i];
        if (res.is_valid()) {
            auto raw_graph = res->get_simulation_graph_ptr();
            if (raw_graph) {
                // Mapping Godot Resource settings to Core ExecutionTiming
                // Logic assumes the Resource wrapper carries its intended timing/interval
                manager->register_managed_graph(
                    raw_graph, 
                    static_cast<ideam::core::ExecutionTiming>(res->get_execution_timing()), 
                    res->get_interval()
                );
                active_graphs.insert(res); // Pinning
            }
        }
    }
}

void SimulationEngine::_clear_core_bindings() {
    // Unregister everything from core to prevent dangling pointers
    for (auto& res : active_graphs) {
        if (res.is_valid()) {
            manager->unregister_managed_graph(res->get_simulation_graph_ptr());
        }
    }
    for (auto& res : active_buffers) {
        if (res.is_valid()) {
            manager->unregister_buffer(res->get_simulation_buffer_ptr());
        }
    }
    
    active_graphs.clear();
    active_buffers.clear();
}

// --- Getters / Setters ---

void SimulationEngine::set_active(bool p_active) {
    active = p_active;
}

bool SimulationEngine::is_active() const {
    return active;
}

void SimulationEngine::set_buffers(TypedArray<SimulationBufferResource> p_buffers) {
    buffers = p_buffers;
}

TypedArray<SimulationBufferResource> SimulationEngine::get_buffers() const {
    return buffers;
}

void SimulationEngine::set_graphs(TypedArray<SimulationGraphResource> p_graphs) {
    graphs = p_graphs;
}

TypedArray<SimulationGraphResource> SimulationEngine::get_graphs() const {
    return graphs;
}

void SimulationEngine::execute_manual(double p_delta) {
    if (manager) {
        manager->execute_manual(p_delta);
    }
}

uint32_t SimulationEngine::get_version() const {
    return manager ? manager->get_version() : 0;
}

} // namespace godot