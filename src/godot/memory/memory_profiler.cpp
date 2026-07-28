#include "memory_profiler.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryProfiler::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_save_pressed"), &MemoryProfiler::_on_save_pressed);
    ClassDB::bind_method(D_METHOD("_on_buffer_item_selected", "index"), &MemoryProfiler::_on_buffer_item_selected);
    ClassDB::bind_method(D_METHOD("_on_managed_buffer_item_selected", "index"), &MemoryProfiler::_on_managed_buffer_item_selected);
    ClassDB::bind_method(D_METHOD("_on_ribbon_inspection_requested", "block_type", "index"), &MemoryProfiler::_on_ribbon_inspection_requested);
    
    ClassDB::bind_method(D_METHOD("_on_theme_applied", "theme", "index"), &MemoryProfiler::_on_theme_applied);

    ADD_SIGNAL(MethodInfo("runtime_save_requested", PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource")));
}

MemoryProfiler::MemoryProfiler() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // --- Row 1: Control Bar ---
    control_bar = memnew(HBoxContainer);
    add_child(control_bar);

    theme_selector_ui = memnew(ThemeSelector);
    // Populate initial state and point it to the graphs theme registry
    String registry_path = "res://addons/ideam_memory/resources/memory_profiler_theme_registry.tres";
    theme_selector_ui->setup(registry_path);

    // Select the default theme at index 0 for the composer UI itself
    
    control_bar->add_child(theme_selector_ui);

    save_btn = memnew(Button);
    save_btn->set_text("Save");
    control_bar->add_child(save_btn);

    theme_selector_ui->connect("theme_applied", Callable(this, "_on_theme_applied"));
    save_btn->connect("pressed", Callable(this, "_on_save_pressed"));

    // --- Row 2: Memory Ribbon ---
    memory_ribbon = memnew(MemoryRibbon);
    add_child(memory_ribbon);
    
    memory_ribbon->connect("inspection_requested", Callable(this, "_on_ribbon_inspection_requested"));

    // --- Row 3: Main Workspace ---
    main_workspace = memnew(HBoxContainer);
    main_workspace->set_h_size_flags(SIZE_EXPAND_FILL);
    main_workspace->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(main_workspace);

    // Column A: Buffer Selection Sidebar
    sidebar = memnew(VBoxContainer);
    sidebar->set_custom_minimum_size(Vector2(200, 0)); 
    sidebar->set_v_size_flags(SIZE_EXPAND_FILL);
    main_workspace->add_child(sidebar);

    Label* sidebar_title = memnew(Label);
    sidebar_title->set_theme_type_variation("HeaderSmall");
    sidebar_title->set_text("Memory Buffers");
    sidebar->add_child(sidebar_title);

    memory_buffer_list = memnew(ItemList);
    memory_buffer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    memory_buffer_list->set_v_size_flags(SIZE_EXPAND_FILL);
    sidebar->add_child(memory_buffer_list);

    memory_buffer_list->connect("item_selected", Callable(this, "_on_buffer_item_selected"));

    managed_buffer_list_title = memnew(Label);
    managed_buffer_list_title->set_theme_type_variation("HeaderSmall");
    managed_buffer_list_title->set_text("Managed Profiles");
    sidebar->add_child(managed_buffer_list_title);

    managed_buffer_list = memnew(ItemList);
    managed_buffer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    managed_buffer_list->set_v_size_flags(SIZE_EXPAND_FILL);
    sidebar->add_child(managed_buffer_list);
    
    managed_buffer_list->connect("item_selected", Callable(this, "_on_managed_buffer_item_selected"));

    // Column B: Primary Visualization View
    view_container = memnew(PanelContainer);
    view_container->set_h_size_flags(SIZE_EXPAND_FILL);
    view_container->set_v_size_flags(SIZE_EXPAND_FILL);
    main_workspace->add_child(view_container);

    memory_buffer_view = memnew(MemoryBufferView);
    view_container->add_child(memory_buffer_view);

    managed_buffer_view = memnew(ManagedBufferView);
    managed_buffer_view->set_visible(false); // Hide by default
    view_container->add_child(managed_buffer_view);

    // Column C: Pseudo-Inspector Panel
    inspector_panel = memnew(PanelContainer);
    inspector_panel->set_custom_minimum_size(Vector2(250, 0));
    inspector_panel->set_v_size_flags(SIZE_EXPAND_FILL);
    main_workspace->add_child(inspector_panel);

    inspector_content = memnew(VBoxContainer);
    inspector_content->set_h_size_flags(SIZE_EXPAND_FILL);
    inspector_content->set_v_size_flags(SIZE_EXPAND_FILL);
    inspector_panel->add_child(inspector_content);

    inspector_title = memnew(Label);
    inspector_title->set_theme_type_variation("HeaderSmall");
    inspector_title->set_text("Inspector");
    inspector_content->add_child(inspector_title);

    Label* placeholder = memnew(Label);
    placeholder->set_text("Select an item to view its telemetry snapshot.");
    placeholder->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    inspector_content->add_child(placeholder);

    theme_selector_ui->select_theme(0); 
}

MemoryProfiler::~MemoryProfiler() {}

void MemoryProfiler::_on_theme_applied(const Ref<Theme>& p_theme, int p_index) {
    if (p_theme.is_valid()) {
        set_theme(p_theme);
    } else {
        set_theme(nullptr); // Clears the theme, falling back to Godot defaults
    }
}

void MemoryProfiler::_on_save_pressed() {
    if (active_resource.is_null()) return;

    // Context Sensitive Routing Layer (mirrors GraphComposer)[cite: 7]
    if (Engine::get_singleton()->is_editor_hint()) {
        String res_path = active_resource->get_path();
        
        if (res_path.is_empty() || res_path.contains("::")) {
            UtilityFunctions::printerr("MemoryProfiler: Cannot save. Resource doesn't have a valid standalone path.");
            return;
        }
        active_resource->serialize_subresources_to_disk();
        Error err = ResourceSaver::get_singleton()->save(active_resource, res_path);
        if (err == OK) {
            UtilityFunctions::print("MemoryProfiler: Successfully saved resource to ", res_path);
        } else {
            UtilityFunctions::printerr("MemoryProfiler: Failed to save resource. Error code: ", err);
        }
    } else {
        UtilityFunctions::print("MemoryProfiler: Standalone runtime detected. Routing to runtime storage pipeline.");
        emit_signal("runtime_save_requested", active_resource);
    }
}

void MemoryProfiler::_on_buffer_item_selected(int p_index) {
    if (active_resource.is_null()) return;

    managed_buffer_list->deselect_all();
    managed_buffer_view->set_visible(false);
    memory_buffer_view->set_visible(true);

    TypedArray<MemoryBufferResource> schemas = active_resource->get_buffer_schemas();
    if (p_index >= 0 && p_index < schemas.size()) {
        Ref<MemoryBufferResource> schema = schemas[p_index];
        if (schema.is_valid()) {
            memory_buffer_view->open_buffer(schema);
            inspector_title->set_text(String("Inspecting: ") + schema->get_buffer_name());
            
            // Future DOD snapshot binding can happen right here to populate Column C
        }
    }
}

void MemoryProfiler::_on_managed_buffer_item_selected(int p_index) {
    if (active_resource.is_null()) return;

    memory_buffer_list->deselect_all();
    memory_buffer_view->set_visible(false);
    managed_buffer_view->set_visible(true);

    TypedArray<ManagedBufferResource> profiles = active_resource->get_managed_buffers();
    if (p_index >= 0 && p_index < profiles.size()) {
        Ref<ManagedBufferResource> profile = profiles[p_index];
        if (profile.is_valid()) {
            managed_buffer_view->open_resource(profile);
            
            String p_name = profile->get_consumer_name();
            inspector_title->set_text(String("Inspecting: ") + (p_name.is_empty() ? "Unnamed Profile" : p_name));
        }
    }
}

void MemoryProfiler::_on_ribbon_inspection_requested(int p_block_type, int p_index) {
    // Forward the visual selection from the memory ribbon to our main workspace logic
    if (p_block_type == MemoryRibbon::BLOCK_BUFFER) {
        if (memory_buffer_list->get_item_count() > p_index) {
            memory_buffer_list->select(p_index);
            _on_buffer_item_selected(p_index);
        }
    } else if (p_block_type == MemoryRibbon::BLOCK_MANAGED) {
       if (managed_buffer_list->get_item_count() > p_index) {
            managed_buffer_list->select(p_index);
            _on_managed_buffer_item_selected(p_index);
        }
    } else if (p_block_type == MemoryRibbon::BLOCK_TRANSIENT) {
        inspector_title->set_text("Inspecting: Transient Capacity");
        memory_buffer_view->open_buffer(Ref<MemoryBufferResource>());
        
        memory_buffer_view->set_visible(false);
        managed_buffer_view->set_visible(false);
    }
}

void MemoryProfiler::_populate_ui() {
    memory_buffer_list->clear();
    managed_buffer_list->clear();

    if (active_resource.is_null()) return;

    TypedArray<MemoryBufferResource> schemas = active_resource->get_buffer_schemas();
    for (int i = 0; i < schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = schemas[i];
        if (schema.is_valid()) {
            memory_buffer_list->add_item(schema->get_buffer_name());
        } else {
            memory_buffer_list->add_item("Invalid Schema");
        }
    }

    TypedArray<ManagedBufferResource> profiles = active_resource->get_managed_buffers();
    for (int i = 0; i < profiles.size(); ++i) {
        Ref<ManagedBufferResource> profile = profiles[i];
        if (profile.is_valid()) {
            String p_name = profile->get_consumer_name();
            managed_buffer_list->add_item(p_name.is_empty() ? "Unnamed Profile" : p_name);
        } else {
            managed_buffer_list->add_item("Invalid Profile");
        }
    }
}

void MemoryProfiler::open_resource(Ref<MemoryManagerResource> p_resource) {
    if (p_resource.is_null()) return;
    active_resource = p_resource;
    
    if (memory_ribbon) {
        memory_ribbon->sync_with_resource(active_resource);
    }

    _populate_ui();
}

void MemoryProfiler::close_resource() {
    active_resource.unref();
    
    memory_buffer_list->clear();
    memory_buffer_view->open_buffer(Ref<MemoryBufferResource>());
    
    managed_buffer_list->clear();
    managed_buffer_view->open_resource(Ref<ManagedBufferResource>());

    if (memory_ribbon) {
        memory_ribbon->sync_with_resource(Ref<MemoryManagerResource>());
    }
}

// --- Static Entry Points (Routing Layer) ---

Window* MemoryProfiler::create_profiler_window() {
    Window* profiler_window = memnew(Window);
    profiler_window->set_title("Memory Profiler");
    profiler_window->set_min_size(Vector2i(900, 600)); // Widened base size to accommodate new 3-column layout
    profiler_window->set_transient(false);
    
    // In runtime, closing the window destroys it to free memory[cite: 7]
    profiler_window->connect("close_requested", Callable(profiler_window, "queue_free"));

    SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (tree && tree->get_root()) {
        tree->get_root()->add_child(profiler_window);
    } else {
        UtilityFunctions::printerr("MemoryProfiler: Cannot attach window, no SceneTree root found.");
    }

    return profiler_window;
}

void MemoryProfiler::profile_memory_manager(Ref<MemoryManagerResource> p_resource, Control* p_owner) {
    if (p_resource.is_null()) return;
    
    MemoryProfiler* target_profiler = nullptr;

    if (p_owner) {
        // Linear scan of explicit parent[cite: 7]
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_profiler = Object::cast_to<MemoryProfiler>(p_owner->get_child(i));
            
            if (target_profiler) break;
        }

        if (!target_profiler) {
            target_profiler = memnew(MemoryProfiler);
            
            p_owner->add_child(target_profiler);
        }
    } else {
        // Environment-aware Window routing[cite: 7]
        Window* target_window = nullptr;
        
        SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree && tree->get_root()) {
            for (int i = 0; i < tree->get_root()->get_child_count(); ++i) {
                Window* w = Object::cast_to<Window>(tree->get_root()->get_child(i));
                
                if (w && w->get_title() == "Memory Profiler") {
                    target_window = w;
                    break;
                }
            }
        }
        
        if (!target_window) {
            target_window = create_profiler_window();
        }

        if (target_window) {
            for (int i = 0; i < target_window->get_child_count(); ++i) {
                target_profiler = Object::cast_to<MemoryProfiler>(target_window->get_child(i));
                
                if (target_profiler) break;
            }

            if (!target_profiler) {
                target_profiler = memnew(MemoryProfiler);
                target_window->add_child(target_profiler);
            }
            
            target_window->popup_centered();
            target_window->grab_focus(); 
        }
    }

    if (target_profiler) {
        target_profiler->open_resource(p_resource);
    }
}

void MemoryProfiler::close_memory_profiler(Ref<MemoryManagerResource> p_resource, Control* p_owner) {
    MemoryProfiler* target_profiler = nullptr;

    // Search for the profiler attached to either the owner or the window
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_profiler = Object::cast_to<MemoryProfiler>(p_owner->get_child(i));
            
            if (target_profiler) break;
        }
    } else {
        SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        
        if (tree && tree->get_root()) {
            for (int i = 0; i < tree->get_root()->get_child_count(); ++i) {
                Window* w = Object::cast_to<Window>(tree->get_root()->get_child(i));
                
                if (w && w->get_title() == "Memory Profiler") {
                    for (int j = 0; j < w->get_child_count(); ++j) {
                        target_profiler = Object::cast_to<MemoryProfiler>(w->get_child(j));
                        
                        if (target_profiler) break;
                    }
                    break;
                }
            }
        }
    }

    if (target_profiler) {
        // If the resource matches what we're editing, clear it out.
        if (target_profiler->active_resource == p_resource) {
            target_profiler->close_resource();
        }
    }
}

} // namespace ideam::godot_ext