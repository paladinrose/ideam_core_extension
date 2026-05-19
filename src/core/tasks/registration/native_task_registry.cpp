#include "native_task_registry.h"

// --- Sub-Registries ---
#include "query_task_registry.h"
#include "transform_task_registry.h"
#include "metadata_task_registry.h"

// --- Manual Tasks ---
#include "../entry_fill_task.h"
#include "../sub_graph_task.h"

// --- Godot Editor Bridging (Resources & UI Graph Nodes) ---
#include "../../../godot/tasks/entry_fill_task_resource.h"
#include "../../../godot/tasks/entry_fill_task_graph_node.h"
#include "../../../godot/tasks/sub_graph_task_resource.h"
#include "../../../godot/tasks/sub_graph_task_graph_node.h"
#include "../../../godot/tasks/task_graph_node.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::core {

godot::HashMap<godot::StringName, NativeTaskFactory>* NativeTaskRegistry::manual_factories = nullptr;
godot::Dictionary* NativeTaskRegistry::ui_utility_matrix = nullptr;

void NativeTaskRegistry::init() {
    // 1. Initialize self
    if (!manual_factories) {
        manual_factories = new godot::HashMap<godot::StringName, NativeTaskFactory>();
        ui_utility_matrix = new godot::Dictionary();
    }

    // 2. Delegate O(1) Matrix Initializations
    // This safely triggers the SubMatrixBuilders to fill pointers and instantiate Godot UI Dictionaries!
    QueryTaskRegistry::init();
    TransformTaskRegistry::init();
    MetadataTaskRegistry::init();
    
    // 3. Register Unique Tasks (Enforced to provide their specific wrapper interfaces)
    register_task<EntryFillTask, godot_ext::EntryFillTaskResource, godot_ext::EntryFillTaskGraphNode>("EntryFillTask"); 
    register_task<SubGraphTask, godot_ext::SubGraphTaskResource, godot_ext::SubGraphTaskGraphNode>("SubGraphTask");
}

std::unique_ptr<INativeTask> NativeTaskRegistry::create(const godot::StringName& p_name) {
    if (manual_factories && manual_factories->has(p_name)) {
        return (*manual_factories)[p_name]();
    }
    
    godot::UtilityFunctions::printerr("NativeTaskRegistry: Failed to create unregistered manual task '", p_name, "'");
    return nullptr;
}

void NativeTaskRegistry::cleanup() {
    // 1. Delegate Cleanup (Flushes Godot UI dictionaries)
    QueryTaskRegistry::cleanup();
    TransformTaskRegistry::cleanup();
    MetadataTaskRegistry::cleanup();
    // SimulationTaskRegistry::cleanup(); // Uncomment when implemented

    // 2. Self Cleanup
    if (manual_factories) {
        delete manual_factories; 
        manual_factories = nullptr;
    }
    if (ui_utility_matrix) {
        delete ui_utility_matrix;
        ui_utility_matrix = nullptr;
    }
}

// --- Routing UI Dictionary Getters to Sub-Registries ---
godot::Dictionary NativeTaskRegistry::get_ui_query_matrix() {
    return (QueryTaskRegistry::ui_query_matrix) ? *QueryTaskRegistry::ui_query_matrix : godot::Dictionary();
}

godot::Dictionary NativeTaskRegistry::get_ui_transform_matrix() {
    return (TransformTaskRegistry::ui_transform_matrix) ? *TransformTaskRegistry::ui_transform_matrix : godot::Dictionary();
}

godot::Dictionary NativeTaskRegistry::get_ui_metadata_matrix() {
    return (MetadataTaskRegistry::ui_metadata_matrix) ? *MetadataTaskRegistry::ui_metadata_matrix : godot::Dictionary();
}

godot::Dictionary NativeTaskRegistry::get_ui_simulation_matrix() {
    // Assuming you have a SimulationTaskRegistry structured identically to the others.
    // return (SimulationTaskRegistry::ui_simulation_matrix) ? *SimulationTaskRegistry::ui_simulation_matrix : godot::Dictionary();
    return godot::Dictionary(); 
}

godot::Dictionary NativeTaskRegistry::get_ui_utility_matrix() {
    return (ui_utility_matrix) ? *ui_utility_matrix : godot::Dictionary();
}

} // namespace ideam::core