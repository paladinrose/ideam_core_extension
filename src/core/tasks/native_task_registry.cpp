#include "native_task_registry.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::core {

godot::HashMap<godot::StringName, NativeTaskFactory> NativeTaskRegistry::factories;

godot::Dictionary NativeTaskRegistry::ui_query_matrix;
godot::Dictionary NativeTaskRegistry::ui_transform_matrix;
godot::Dictionary NativeTaskRegistry::ui_metadata_matrix;
godot::Dictionary NativeTaskRegistry::ui_simulation_matrix;

std::unique_ptr<INativeTask> NativeTaskRegistry::create(const godot::StringName& p_name) {
    if (factories.has(p_name)) {
        return factories[p_name]();
    }
    
    godot::UtilityFunctions::printerr("NativeTaskRegistry: Failed to create unregistered task '", p_name, "'");
    return nullptr;
}

void NativeTaskRegistry::cleanup() {
    factories.clear();
    ui_query_matrix.clear();
    ui_transform_matrix.clear();
    ui_metadata_matrix.clear();
    ui_simulation_matrix.clear();
}

} // namespace ideam::core