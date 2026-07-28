#include "graph_composer.h"
#include "ideam_graphs_plugin.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void GraphComposer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_graph", "graph_edit"), &GraphComposer::open_graph);
    ClassDB::bind_method(D_METHOD("close_graph", "graph_edit"), &GraphComposer::close_graph);

    // Bound to the new unified signal and signature
    ClassDB::bind_method(D_METHOD("_on_tab_changed", "tab"), &GraphComposer::_on_tab_changed);
    ClassDB::bind_method(D_METHOD("_on_theme_applied", "theme", "index"), &GraphComposer::_on_theme_applied);
    ClassDB::bind_method(D_METHOD("_on_save_pressed"), &GraphComposer::_on_save_pressed);
}

GraphComposer::GraphComposer() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // UI Layout Construction
    header_bar = memnew(HBoxContainer);
    add_child(header_bar);

    // Instantiate and embed the generalized ThemeSelector component
    theme_selector_ui = memnew(ThemeSelector);
    header_bar->add_child(theme_selector_ui);

    save_btn = memnew(Button);
    save_btn->set_text("Save Graph");
    header_bar->add_child(save_btn);

    tab_container = memnew(TabContainer);
    tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(tab_container);

    // Event Handling (Signals)
    tab_container->connect("tab_changed", Callable(this, "_on_tab_changed"));
    save_btn->connect("pressed", Callable(this, "_on_save_pressed"));
    
    // Connect to our new custom signal on the selector
    theme_selector_ui->connect("theme_applied", Callable(this, "_on_theme_applied"));

    // Populate initial state and point it to the graphs theme registry
    String registry_path = "res://addons/ideam_graphs/resources/theme_registry.tres";
    theme_selector_ui->setup(registry_path);

    // Select the default theme at index 0 for the composer UI itself
    theme_selector_ui->select_theme(0); 

    active_sessions.reserve(8); 
}

GraphComposer::~GraphComposer() {}

void GraphComposer::_notification(int p_what) {}

// --- State Management & Synchronization ---

void GraphComposer::_on_tab_changed(int p_tab) {
    // Context Sensitivity: Sync dropdown visually with the current tab
    for (const auto& session : active_sessions) {
        if (session.tab_index == p_tab) {
            theme_selector_ui->select_theme(session.theme_index);
            break;
        }
    }
}

void GraphComposer::_on_theme_applied(const Ref<Theme>& p_theme, int p_index) {
    int current_tab = tab_container->get_current_tab();
    
    // If no tabs are open, assume we are styling the Composer UI itself
    if (current_tab < 0) {
        _apply_default_composer_theme(p_theme);
        return;
    }

    // Otherwise, apply the theme to the currently active graph editing session
    for (auto& session : active_sessions) {
        if (session.tab_index == current_tab) {
            session.theme_index = p_index;

            if (p_theme.is_valid()) {
                session.editor_node->set_theme(p_theme);
            } else {
                session.editor_node->set_theme(nullptr); // Fallback to Godot default
            }
            break;
        }
    }
}

void GraphComposer::_apply_default_composer_theme(const Ref<Theme>& p_theme) {
    if (p_theme.is_valid()) {
        set_theme(p_theme); 
    } else {
        set_theme(nullptr);
    }
}

void GraphComposer::_on_save_pressed() {
    int current_tab = tab_container->get_current_tab();
    if (current_tab < 0) return;

    // Find the current active session
    IdeamGraphEdit* active_edit = nullptr;

    for (const auto& session : active_sessions) {
        if (session.tab_index == current_tab) {
            active_edit = session.editor_node;
            break;
        }
    }

    if (!active_edit) return;

    Ref<IdeamGraphResource> blueprint = active_edit->get_blueprint();
    if (blueprint.is_null()) return;

    // --- CONTEXT SENSITIVE ROUTING LAYER ---
    if (Engine::get_singleton()->is_editor_hint()) {
        String res_path = blueprint->get_path();
        
        if (res_path.is_empty() || res_path.contains("::")) {
            UtilityFunctions::printerr("GraphComposer: Cannot save. Graph resource doesn't have a valid standalone path (res://...)");
            return;
        }
        blueprint->get_memory_manager()->serialize_subresources_to_disk();
        Error err = ResourceSaver::get_singleton()->save(blueprint, res_path);
        if (err == OK) {
            UtilityFunctions::print("GraphComposer: Successfully saved graph resource to ", res_path);
        } else {
            UtilityFunctions::printerr("GraphComposer: Failed to save graph resource. Error code: ", err);
        }
    } else {
        UtilityFunctions::print("GraphComposer: Standalone runtime detected. Routing to runtime storage pipeline.");
        
        if (has_signal("runtime_save_requested")) {
            emit_signal("runtime_save_requested", active_edit);
        }
    }
}

void GraphComposer::open_graph(IdeamGraphEdit* p_graph_edit) {
    if (!p_graph_edit) return;

    Ref<IdeamGraphResource> blueprint = p_graph_edit->get_blueprint();
    if (blueprint.is_null()) {
        UtilityFunctions::printerr("GraphComposer: Attempted to open graph with null blueprint.");
        p_graph_edit->queue_free(); 
        return; 
    }

    const auto* target_key = blueprint.ptr();

    for (const auto& session : active_sessions) {
        if (session.resource_key == target_key) {
            tab_container->set_current_tab(session.tab_index);
            p_graph_edit->queue_free(); 
            return; 
        }
    }

    String node_name = blueprint->get_name().is_empty() ? "Untitled Graph" : blueprint->get_name();
    p_graph_edit->set_name(node_name);
    
    tab_container->add_child(p_graph_edit);
    int new_index = tab_container->get_tab_count() - 1;
    tab_container->set_current_tab(new_index);

    // Initialize with theme_index 0 (Default)
    active_sessions.push_back({target_key, p_graph_edit, new_index, 0});
    
    // Force UI to sync with the newly opened tab
    _on_tab_changed(new_index);
}

void GraphComposer::close_graph(IdeamGraphEdit* p_graph_edit) {
    if (!p_graph_edit) return;

    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it) {
        if (it->editor_node == p_graph_edit) {
            tab_container->remove_child(p_graph_edit);
            p_graph_edit->queue_free();
            active_sessions.erase(it);

            for (int i = 0; i < active_sessions.size(); ++i) {
                active_sessions[i].tab_index = i;
            }
            break;
        }
    }
}

// --- Static Entry Points (Routing Layer) ---

Window* GraphComposer::create_runtime_composer_window() {
    Window* runtime_window = memnew(Window);
    runtime_window->set_title("Runtime Graph Composer");
    runtime_window->set_min_size(Vector2i(800, 600));
    runtime_window->set_transient(false);
    
    // In runtime, closing the window destroys it to free memory
    runtime_window->connect("close_requested", Callable(runtime_window, "queue_free"));

    // Anchor it to the main SceneTree root 
    SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (tree && tree->get_root()) {
        tree->get_root()->add_child(runtime_window);
    } else {
        UtilityFunctions::printerr("GraphComposer: Cannot attach runtime window, no SceneTree root found.");
    }

    return runtime_window;
}

void GraphComposer::edit_ideam_graph(IdeamGraphEdit* p_graph_edit, Control* p_owner) {
    GraphComposer* target_composer = nullptr;

    if (p_owner) {
        // Linear scan of explicit parent
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_composer = Object::cast_to<GraphComposer>(p_owner->get_child(i));
            if (target_composer) break;
        }

        if (!target_composer) {
            target_composer = memnew(GraphComposer);
            p_owner->add_child(target_composer);
        }
    } else {
        // Environment-aware Window routing
        Window* target_window = nullptr;

        if (Engine::get_singleton()->is_editor_hint()) {
            target_window = IdeamGraphsPlugin::get_shared_composer_window();
        } else {
            // Find existing runtime window to avoid spamming multiple popups
            SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
            if (tree && tree->get_root()) {
                for (int i = 0; i < tree->get_root()->get_child_count(); ++i) {
                    Window* w = Object::cast_to<Window>(tree->get_root()->get_child(i));
                    if (w && w->get_title() == "Runtime Graph Composer") {
                        target_window = w;
                        break;
                    }
                }
            }
            
            // Spawn if none found
            if (!target_window) {
                target_window = create_runtime_composer_window();
            }
        }

        if (target_window) {
            
            // Check if the window already has our Composer child
            for (int i = 0; i < target_window->get_child_count(); ++i) {
                target_composer = Object::cast_to<GraphComposer>(target_window->get_child(i));
                if (target_composer) break;
            }

            if (!target_composer) {
                target_composer = memnew(GraphComposer);
                target_window->add_child(target_composer);
            }
            
            // Pop the window to the front
            target_window->popup_centered();
            target_window->grab_focus(); 
        }
    }

    // Execute the instantiation request
    if (target_composer) {
        target_composer->open_graph(p_graph_edit);
    } else {
        // Abort safely
        p_graph_edit->queue_free();
    }
}

void GraphComposer::close_ideam_graph(IdeamGraphEdit* p_graph_edit, Control* p_owner) {
    GraphComposer* target_composer = nullptr;

    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_composer = Object::cast_to<GraphComposer>(p_owner->get_child(i));
            if (target_composer) break;
        }
    } else {
        Window* target_window = nullptr;
        
        if (Engine::get_singleton()->is_editor_hint()) {
            target_window = IdeamGraphsPlugin::get_shared_composer_window();
        } else {
            SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
            if (tree && tree->get_root()) {
                for (int i = 0; i < tree->get_root()->get_child_count(); ++i) {
                    Window* w = Object::cast_to<Window>(tree->get_root()->get_child(i));
                    if (w && w->get_title() == "Runtime Graph Composer") {
                        target_window = w;
                        break;
                    }
                }
            }
        }

        if (target_window) {
            for (int i = 0; i < target_window->get_child_count(); ++i) {
                target_composer = Object::cast_to<GraphComposer>(target_window->get_child(i));
                if (target_composer) break;
            }
        }
    }

    if (target_composer) {
        target_composer->close_graph(p_graph_edit);
    }
}

} // namespace ideam::godot_ext