#include "ideam_graphs_plugin.h"
#include "ideam_graph_inspector.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

IdeamGraphsPlugin *IdeamGraphsPlugin::singleton = nullptr;

void IdeamGraphsPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_composer_window_closed"), &IdeamGraphsPlugin::_on_composer_window_closed);
}

IdeamGraphsPlugin::IdeamGraphsPlugin() {
    singleton = this;
}

IdeamGraphsPlugin::~IdeamGraphsPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamGraphsPlugin::_enter_tree() {
    validate_script_templates();
    
    // 1. Handshake: Announce this plugin is active
    set_plugin_active("IdeamGraphs", true);

    // 2. Registry: Register the graph scaffolding settings into the global Wizard ecosystem
    register_to_ecosystem("WizardSettings", "IdeamGraphs", String(GRAPHS_SETTINGS_PATH.data()));

    // Instantiate and register the Inspector UI for the Graph Resources
    graph_editor = Ref<IdeamGraphInspector>(memnew(IdeamGraphInspector));
    add_inspector_plugin(graph_editor);
}

void IdeamGraphsPlugin::_exit_tree() {
    remove_inspector_plugin(graph_editor);
    
    // Cleanup the editor window if it's still alive
    if (graph_composer_window) {
        graph_composer_window->queue_free();
        graph_composer_window = nullptr;
    }

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamGraphs", false);
}

void IdeamGraphsPlugin::_on_composer_window_closed() {
    if (graph_composer_window) {
        graph_composer_window->hide();
    }
}

Object *IdeamGraphsPlugin::undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}

Window* IdeamGraphsPlugin::get_shared_composer_window() {
    if (!singleton) {
        return nullptr;
    }

    // Lazy initialization of the shared window
    if (!singleton->graph_composer_window) {
        singleton->graph_composer_window = memnew(Window);
        singleton->graph_composer_window->set_title("Graph Composer");
        singleton->graph_composer_window->set_min_size(Vector2i(800, 600));
        
        // --- Window Decoration & Interaction Configurations ---
        
        // Mark as transient to link its lifecycle and visibility context to the editor main window
        singleton->graph_composer_window->set_transient(true);
        
        // Disable exclusivity so the user can interact with both the editor and this window simultaneously
        singleton->graph_composer_window->set_exclusive(false);
        
        // Connect the close button event to the plugin's hide routing
        singleton->graph_composer_window->connect("close_requested", Callable(singleton, "_on_composer_window_closed"));
        
        // If we are in the editor, we can parent it to the editor's base control
        if (Engine::get_singleton()->is_editor_hint()) {
            EditorInterface::get_singleton()->get_base_control()->add_child(singleton->graph_composer_window);
        }
    }

    return singleton->graph_composer_window;
}

} // namespace ideam::godot_ext