#include "ideam_tasks_plugin.h"
#include "task_graph_inspector.h"
#include "../../core/tasks/registration/ideam_task_registry.h" // Updated include

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamTasksPlugin *IdeamTasksPlugin::singleton = nullptr;

void IdeamTasksPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_manifest_updated"), &IdeamTasksPlugin::_on_manifest_updated);
}

IdeamTasksPlugin::IdeamTasksPlugin() {
    singleton = this;
}

IdeamTasksPlugin::~IdeamTasksPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamTasksPlugin::_enter_tree() {
    validate_script_templates();
    
    // 1. Handshake: Announce the tasks plugin is active in the ecosystem
    set_plugin_active("IdeamTasks", true);

    // 2. Registry: Register the tasks scaffolding settings into the global Wizard ecosystem
    register_to_ecosystem("WizardSettings", "IdeamTasks", String(TASKS_SETTINGS_PATH.data()));

    // 3. Setup Task Registry Tools and UI
    auto registry = core::IdeamTaskRegistry::get_singleton();
    if (registry) { // Singleton pointers use standard pointer validation
        // Connect to rebake signals so the Editor can heal itself dynamically
        registry->connect("manifest_updated", Callable(this, "_on_manifest_updated"));
        
        // Expose tool menu item bridging to the IdeamTaskRegistry Singleton
        add_tool_menu_item("Update Task Manifest", Callable(registry, "bake_manifest"));
    }

    _check_manifest_and_setup();
}

void IdeamTasksPlugin::_exit_tree() {
    remove_tool_menu_item("Update Task Manifest");

    // Clean teardown of the inspector plugin
    if (manifest_valid && task_inspector.is_valid()) {
        remove_inspector_plugin(task_inspector);
    }

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamTasks", false);
}

void IdeamTasksPlugin::_on_manifest_updated() {
    _check_manifest_and_setup();
}

void IdeamTasksPlugin::_check_manifest_and_setup() {
    auto registry = core::IdeamTaskRegistry::get_singleton();
    
    if (registry && registry->get_manifest_version() == core::TaskManifest::CURRENT_MANIFEST_VERSION) {
        if (!manifest_valid) {
            manifest_valid = true;
            task_inspector = Ref<TaskGraphInspector>(memnew(TaskGraphInspector));
            add_inspector_plugin(task_inspector);
            UtilityFunctions::print("IdeamTasks: Manifest validated. Task Graph Editing is enabled.");
        }
    } else {
        if (manifest_valid) {
            manifest_valid = false;
            remove_inspector_plugin(task_inspector);
            task_inspector.unref();
        }
        UtilityFunctions::push_warning("IdeamTasks: Task Manifest version mismatch or missing. Please go to Project > Tools > Update Task Manifest. Task Graph Editing is currently disabled.");
    }
}

Object *IdeamTasksPlugin::undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}

} // namespace ideam::godot_ext