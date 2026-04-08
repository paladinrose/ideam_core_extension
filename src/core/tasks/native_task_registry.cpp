#include "native_task_registry.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::core {

godot::HashMap<godot::StringName, NativeTaskFactory>* NativeTaskRegistry::factories = nullptr;

godot::Dictionary* NativeTaskRegistry::ui_query_matrix = nullptr;
godot::Dictionary* NativeTaskRegistry::ui_transform_matrix = nullptr;
godot::Dictionary* NativeTaskRegistry::ui_metadata_matrix = nullptr;
godot::Dictionary* NativeTaskRegistry::ui_simulation_matrix = nullptr;

void NativeTaskRegistry::init() {
    if (!factories) factories = new godot::HashMap<godot::StringName, NativeTaskFactory>();
    if (!ui_query_matrix) ui_query_matrix = new godot::Dictionary();
    if (!ui_transform_matrix) ui_transform_matrix = new godot::Dictionary();
    if (!ui_metadata_matrix) ui_metadata_matrix = new godot::Dictionary();
    if (!ui_simulation_matrix) ui_simulation_matrix = new godot::Dictionary();
}

std::unique_ptr<INativeTask> NativeTaskRegistry::create(const godot::StringName& p_name) {
    if (factories && factories->has(p_name)) {
        return (*factories)[p_name]();
    }
    
    godot::UtilityFunctions::printerr("NativeTaskRegistry: Failed to create unregistered task '", p_name, "'");
    return nullptr;
}

void NativeTaskRegistry::cleanup() {
    delete factories; factories = nullptr;
    delete ui_query_matrix; ui_query_matrix = nullptr;
    delete ui_transform_matrix; ui_transform_matrix = nullptr;
    delete ui_metadata_matrix; ui_metadata_matrix = nullptr;
    delete ui_simulation_matrix; ui_simulation_matrix = nullptr;
}

} // namespace ideam::core