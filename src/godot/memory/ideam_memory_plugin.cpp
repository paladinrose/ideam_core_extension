#include "ideam_memory_plugin.h"
#include "memory_graph_inspector.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

IdeamMemoryPlugin *IdeamMemoryPlugin::singleton = nullptr;

void IdeamMemoryPlugin::_bind_methods() {
    // Stripped of composer methods; cleanly delegates UI heavy lifting to the Graphs plugin
}

IdeamMemoryPlugin::IdeamMemoryPlugin() {
    singleton = this;
}

IdeamMemoryPlugin::~IdeamMemoryPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamMemoryPlugin::_enter_tree() {
    validate_script_templates();
    
    // 1. Handshake: Announce the memory manager DOD plugin is active
    set_plugin_active("IdeamMemory", true);

    // 2. Registry: Register the memory scaffolding settings into the global Wizard ecosystem
    register_to_ecosystem("WizardSettings", "IdeamMemory", String(MEMORY_SETTINGS_PATH.data()));

    // Instantiate and register the Inspector UI for Memory Resources
    // This allows us to inspect MemoryBufferPODs and MemoryGrantPODs in the editor
    memory_inspector = Ref<MemoryGraphInspector>(memnew(MemoryGraphInspector));
    add_inspector_plugin(memory_inspector);
}

void IdeamMemoryPlugin::_exit_tree() {
    // Clean teardown of the inspector plugin
    remove_inspector_plugin(memory_inspector);

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamMemory", false);
}

Object *IdeamMemoryPlugin::undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}

} // namespace ideam::godot_ext