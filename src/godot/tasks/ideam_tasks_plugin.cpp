#include "ideam_tasks_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
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
    // 1. Filesystem & Registry Handshake
    validate_script_templates();
    check_for_project_tools(String(TASKS_SETTINGS_PATH.data()), String(SETTINGS_PATHS.data()));

    // 2. Initialize Inspector Plugin
    // NOTE: This assumes we will implement IdeamTaskPlanInspector as a GDExtension class.
    // If it remains a GDScript, we would load it via ResourceLoader here.
    // For now, we instantiate the placeholder reference.
    // plan_editor.instantiate(); 
    // add_inspector_plugin(plan_editor);
}

void IdeamTasksPlugin::_exit_tree() {
    if (plan_editor.is_valid()) {
        remove_inspector_plugin(plan_editor);
    }
}

Object *IdeamTasksPlugin::undo_redo() {
    if (singleton) {
        // Accessing the native EditorPlugin::get_undo_redo()
        return singleton->get_undo_redo();
    }
    return nullptr;
}

void IdeamTasksPlugin::wait_for_editor_frame() {
    if (singleton && singleton->get_tree()) {
        // Mirroring 'await it.get_tree().process_frame' logic
        // In C++, this is usually handled via call_deferred or signals,
        // but this static hook remains for logic parity.
    }
}

} // namespace godot