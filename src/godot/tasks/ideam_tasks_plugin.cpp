#include "ideam_tasks_plugin.h"
#include "task_graph_inspector.h" // Integrating the inspector we just built

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

IdeamTasksPlugin *IdeamTasksPlugin::singleton = nullptr;

void IdeamTasksPlugin::_bind_methods() {
    // Methods intended for internal or GDScript usage can be bound here.
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

    // 3. Instantiate and register the localized Inspector UI for Task Resources
    task_inspector.instantiate(); 
    add_inspector_plugin(task_inspector);
}

void IdeamTasksPlugin::_exit_tree() {
    // Clean teardown of the inspector plugin
    if (task_inspector.is_valid()) {
        remove_inspector_plugin(task_inspector);
    }

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamTasks", false);
}

Object *IdeamTasksPlugin::undo_redo() {
    if (singleton) {
        // Accessing the native EditorPlugin::get_undo_redo() securely
        return singleton->get_undo_redo();
    }
    return nullptr;
}

} // namespace godot