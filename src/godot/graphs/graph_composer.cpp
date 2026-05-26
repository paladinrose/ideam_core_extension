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

    // Bind UI Signals
    ClassDB::bind_method(D_METHOD("_on_tab_changed", "tab"), &GraphComposer::_on_tab_changed);
    ClassDB::bind_method(D_METHOD("_on_theme_selected", "index"), &GraphComposer::_on_theme_selected);
    ClassDB::bind_method(D_METHOD("_on_load_theme_pressed"), &GraphComposer::_on_load_theme_pressed);
    ClassDB::bind_method(D_METHOD("_on_theme_file_selected", "path"), &GraphComposer::_on_theme_file_selected);
    ClassDB::bind_method(D_METHOD("_on_save_pressed"), &GraphComposer::_on_save_pressed);
}

GraphComposer::GraphComposer() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // 1. Load the Theme Registry
    theme_registry = ThemeRegistry::load_registry();

    // 2. UI Layout Construction
    header_bar = memnew(HBoxContainer);
    add_child(header_bar);

    theme_selector = memnew(OptionButton);
    theme_selector->set_h_size_flags(SIZE_EXPAND_FILL);
    header_bar->add_child(theme_selector);

    load_theme_btn = memnew(Button);
    load_theme_btn->set_text("Load Theme");
    header_bar->add_child(load_theme_btn);

    save_btn = memnew(Button);
    save_btn->set_text("Save Graph");
    header_bar->add_child(save_btn);

    tab_container = memnew(TabContainer);
    tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(tab_container);

    // Setup FileDialog for Theme loading
    theme_file_dialog = memnew(FileDialog);
    theme_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    theme_file_dialog->set_access(FileDialog::ACCESS_RESOURCES); // Matches res:// context
    PackedStringArray filters;
    filters.append("*.tres, *.theme ; Godot Theme Files");
    theme_file_dialog->set_filters(filters);
    add_child(theme_file_dialog);

    // 3. Event Handling (Signals)
    tab_container->connect("tab_changed", Callable(this, "_on_tab_changed"));
    theme_selector->connect("item_selected", Callable(this, "_on_theme_selected"));
    load_theme_btn->connect("pressed", Callable(this, "_on_load_theme_pressed"));
    save_btn->connect("pressed", Callable(this, "_on_save_pressed"));
    theme_file_dialog->connect("file_selected", Callable(this, "_on_theme_file_selected"));

    // Populate initial state
    _refresh_theme_list();
    active_sessions.reserve(8); 
}

GraphComposer::~GraphComposer() {}

void GraphComposer::_notification(int p_what) {}

// --- State Management & Synchronization ---

void GraphComposer::_refresh_theme_list() {
    theme_selector->clear();
    
    // Add default entry
    theme_selector->add_item("Default Theme");
    theme_selector->set_item_metadata(0, ""); // Empty path for default
    
    theme_selector->add_separator();

    TypedArray<String> paths = theme_registry->get_theme_paths();
    for (int i = 0; i < paths.size(); ++i) {
        String path = paths[i];
        // The index will auto-increment. We use get_file() for a clean UI name.
        theme_selector->add_item(path.get_file()); 
        
        // Store the absolute path in the item's metadata for easy retrieval
        int item_idx = theme_selector->get_item_count() - 1;
        theme_selector->set_item_metadata(item_idx, path);
    }
}

void GraphComposer::_on_tab_changed(int p_tab) {
    // Context Sensitivity: Sync dropdown visually with the current tab
    for (const auto& session : active_sessions) {
        if (session.tab_index == p_tab) {
            theme_selector->select(session.theme_index);
            break;
        }
    }
}

void GraphComposer::_on_theme_selected(int p_index) {
    int current_tab = tab_container->get_current_tab();
    if (current_tab < 0) return;

    // Apply the theme and update DOD state tracker
    for (auto& session : active_sessions) {
        if (session.tab_index == current_tab) {
            session.theme_index = p_index;

            if (p_index == 0) {
                // Strip custom theme
                session.editor_node->set_theme(nullptr); 
            } else {
                // Apply custom theme
                String theme_path = theme_selector->get_item_metadata(p_index);
                Ref<Theme> loaded_theme = ResourceLoader::get_singleton()->load(theme_path);
                
                if (loaded_theme.is_valid()) {
                    session.editor_node->set_theme(loaded_theme);
                } else {
                    UtilityFunctions::printerr("GraphComposer: Failed to load theme at ", theme_path);
                }
            }
            break;
        }
    }
}

void GraphComposer::_on_load_theme_pressed() {
    theme_file_dialog->popup_centered_ratio(0.5);
}

void GraphComposer::_on_theme_file_selected(const String& p_path) {
    // Add, save, and refresh UI
    theme_registry->add_theme_path(p_path);
    theme_registry->save_registry();
    _refresh_theme_list();
    
    // Automatically select the newly added theme for the current tab
    int new_index = theme_selector->get_item_count() - 1;
    theme_selector->select(new_index);
    _on_theme_selected(new_index);
}

void GraphComposer::_on_save_pressed() {
    int current_tab = tab_container->get_current_tab();
    if (current_tab < 0) return;

    // Find the current active session
    IdeamGraphEdit* active_edit = nullptr;
    const IdeamGraphResource* resource_key = nullptr;

    for (const auto& session : active_sessions) {
        if (session.tab_index == current_tab) {
            active_edit = session.editor_node;
            resource_key = session.resource_key;
            break;
        }
    }

    if (!active_edit) return;

    Ref<IdeamGraphResource> blueprint = active_edit->get_blueprint();
    if (blueprint.is_null()) return;

    // --- CONTEXT SENSITIVE ROUTING LAYER ---
    if (Engine::get_singleton()->is_editor_hint()) {
        // Editor Context: Use native ResourceSaver
        String res_path = blueprint->get_path();
        
        if (res_path.is_empty() || res_path.contains("::")) {
            // Edge-case handling: Resource was created anonymously or as a transient sub-resource
            UtilityFunctions::printerr("GraphComposer: Cannot save. Graph resource doesn't have a valid standalone path (res://...)");
            return;
        }

        Error err = ResourceSaver::get_singleton()->save(blueprint, res_path);
        if (err == OK) {
            UtilityFunctions::print("GraphComposer: Successfully saved graph resource to ", res_path);
        } else {
            UtilityFunctions::printerr("GraphComposer: Failed to save graph resource. Error code: ", err);
        }
    } else {
        // Runtime Context: Delegate behavior to the game application layer
        UtilityFunctions::print("GraphComposer: Standalone runtime detected. Routing to runtime storage pipeline.");
        
        // Custom Hook: You can emit a custom signal here that your game code listens to,
        // or check if a dedicated script hook is bound to handle JSON/binary serialization.
        if (has_signal("runtime_save_requested")) {
            emit_signal("runtime_save_requested", active_edit);
        }
    }
}
// --- Original Operations ---

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