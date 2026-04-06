#include "ideam_graphs_plugin.h"
#include "ideam_graph_inspector.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot {

IdeamGraphsPlugin *IdeamGraphsPlugin::singleton = nullptr;

void IdeamGraphsPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_graph_composer"), &IdeamGraphsPlugin::open_graph_composer);
    ClassDB::bind_method(D_METHOD("close_graph_composer"), &IdeamGraphsPlugin::close_graph_composer);
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
    graph_editor.instantiate();
    add_inspector_plugin(graph_editor);
}

void IdeamGraphsPlugin::_exit_tree() {
    remove_inspector_plugin(graph_editor);
    close_graph_composer();

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamGraphs", false);
}

void IdeamGraphsPlugin::open_graph_composer() {
    if (graph_composer_window) {
        graph_composer_window->grab_focus();
        return;
    }

    Window *top = EditorInterface::get_singleton()->get_editor_main_screen()->get_window();
    if (!top) return;

    graph_composer_window = memnew(Window);
    graph_composer_window->set_title("Ideam Graph Composer");
    graph_composer_window->connect("close_requested", Callable(this, "close_graph_composer"));

    Vector2i min_size(800, 600);
    graph_composer_window->set_min_size(min_size);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_anchors_preset(Control::PRESET_FULL_RECT);
    graph_composer_window->add_child(scroll);

    Ref<PackedScene> composer_scene = ResourceLoader::get_singleton()->load(String(GRAPH_COMPOSER_SCENE_PATH.data()));
    if (composer_scene.is_valid()) {
        graph_composer = Object::cast_to<Control>(composer_scene->instantiate());
        if (graph_composer) {
            scroll->add_child(graph_composer);
            // Calling GDScript/Dynamic methods via call
            graph_composer->call("build_graph_composer");
        }
    }

    top->add_child(graph_composer_window);
    graph_composer_window->popup_exclusive_centered(top, min_size);
}

void IdeamGraphsPlugin::close_graph_composer() {
    if (graph_composer && graph_composer->has_method("close_tool")) {
        graph_composer->call("close_tool");
    }

    if (graph_composer_window) {
        graph_composer_window->hide();
        graph_composer_window->queue_free();
        graph_composer_window = nullptr;
        graph_composer = nullptr;
    }
}

void IdeamGraphsPlugin::edit_ideam_graph(Object *p_graph, const Callable &p_graph_close) {
    if (!graph_composer_window) {
        open_graph_composer();
    }
    
    if (graph_composer && graph_composer->has_method("open_graph")) {
        Array args;
        args.append(p_graph);
        args.append(p_graph_close);
        graph_composer->callv("open_graph", args);
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
        
        // If we are in the editor, we can parent it to the editor's base control
        if (Engine::get_singleton()->is_editor_hint()) {
            EditorInterface::get_singleton()->get_base_control()->add_child(singleton->graph_composer_window);
        }
    }

    return singleton->graph_composer_window;
}

} // namespace godot