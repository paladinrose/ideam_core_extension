#include "ideam_graphs_plugin.h"

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
    check_for_project_tools(String(GRAPHS_SETTINGS_PATH.data()), String(SETTINGS_PATHS.data()));

    // Expects IdeamGraphInspector to be defined/registered in the C++ layer
    graph_editor.instantiate();
    add_inspector_plugin(graph_editor);

    add_tool_menu_item("Ideam/Graph Composer", callable_mp(this, &IdeamGraphsPlugin::open_graph_composer));
}

void IdeamGraphsPlugin::_exit_tree() {
    remove_inspector_plugin(graph_editor);
    remove_tool_menu_item("Ideam/Graph Composer");

    if (graph_composer_window) {
        graph_composer_window->queue_free();
        graph_composer_window = nullptr;
    }
}

void IdeamGraphsPlugin::open_graph_composer() {
    if (graph_composer_window) {
        graph_composer_window->grab_focus();
        return;
    }

    EditorInterface *ei = get_editor_interface();
    Control *top = Object::cast_to<Control>(ei->get_editor_main_screen());
    
    if (!top) {
        return;
    }
    // Recursive parent search for root control to center popup correctly
    Control *p = top->get_parent_control();
    while (p != nullptr) {
        top = p;
        p = top->get_parent_control();
    }

    graph_composer_window = memnew(Window);
    graph_composer_window->set_title("Ideam Graph Composer");
    
    Vector2 min_size = get_viewport()->get_visible_rect().size * 0.8;
    graph_composer_window->set_min_size(min_size);
    
    graph_composer_window->connect("close_requested", callable_mp(this, &IdeamGraphsPlugin::close_graph_composer));
    
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_custom_minimum_size(min_size);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    graph_composer_window->add_child(scroll);

    Ref<PackedScene> composer_scene = ResourceLoader::get_singleton()->load(String(GRAPH_COMPOSER_SCENE_PATH.data()));
    if (composer_scene.is_valid()) {
        graph_composer = cast_to<Control>(composer_scene->instantiate());
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

void IdeamGraphsPlugin::edit_graph(Object *p_graph, const Callable &p_graph_close) {
    if (singleton) {
        singleton->edit_ideam_graph(p_graph, p_graph_close);
    }
}

} // namespace godot