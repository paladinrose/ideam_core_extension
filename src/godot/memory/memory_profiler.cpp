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
    ClassDB::bind_method(D_METHOD("_on_grant_item_selected", "index"), &MemoryProfiler::_on_grant_item_selected);
    
    ClassDB::bind_method(D_METHOD("_on_grant_list_gui_input", "event"), &MemoryProfiler::_on_grant_list_gui_input);
    ClassDB::bind_method(D_METHOD("_on_grant_list_mouse_exited"), &MemoryProfiler::_on_grant_list_mouse_exited);

    ClassDB::bind_method(D_METHOD("_on_ribbon_inspection_requested", "block_type", "index"), &MemoryProfiler::_on_ribbon_inspection_requested);
    
    ClassDB::bind_method(D_METHOD("_on_theme_applied", "theme", "index"), &MemoryProfiler::_on_theme_applied);
    
    // Bind internal methods for undo/redo callables
    ClassDB::bind_method(D_METHOD("_open_resource", "resource"), &MemoryProfiler::_open_resource);
    ClassDB::bind_method(D_METHOD("_close_resource"), &MemoryProfiler::_close_resource);

    ADD_SIGNAL(MethodInfo("runtime_save_requested", PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource")));
}

MemoryProfiler::MemoryProfiler() {
    undo_redo.instantiate();
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // --- Row 1: Control Bar ---
    control_bar = memnew(HBoxContainer);
    add_child(control_bar);

    theme_selector_ui = memnew(ThemeSelector);
    String registry_path = "res://addons/ideam_memory/resources/memory_profiler_theme_registry.tres";
    theme_selector_ui->setup(registry_path);
    
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

    // Column A: Buffer Selection Sidebar (Now a TabContainer)
    sidebar_tabs = memnew(TabContainer);
    sidebar_tabs->set_custom_minimum_size(Vector2(200, 0)); 
    sidebar_tabs->set_v_size_flags(SIZE_EXPAND_FILL);
    main_workspace->add_child(sidebar_tabs);

    memory_buffer_list = memnew(ItemList);
    memory_buffer_list->set_name("Buffers"); // Sets the Tab Label
    memory_buffer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    memory_buffer_list->set_v_size_flags(SIZE_EXPAND_FILL);
    sidebar_tabs->add_child(memory_buffer_list);
    memory_buffer_list->connect("item_selected", Callable(this, "_on_buffer_item_selected"));

    managed_buffer_list = memnew(ItemList);
    managed_buffer_list->set_name("Managed");
    managed_buffer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    managed_buffer_list->set_v_size_flags(SIZE_EXPAND_FILL);
    sidebar_tabs->add_child(managed_buffer_list);
    managed_buffer_list->connect("item_selected", Callable(this, "_on_managed_buffer_item_selected"));

    memory_grant_list = memnew(ItemList);
    memory_grant_list->set_name("Grants");
    memory_grant_list->set_h_size_flags(SIZE_EXPAND_FILL);
    memory_grant_list->set_v_size_flags(SIZE_EXPAND_FILL);
    sidebar_tabs->add_child(memory_grant_list);
    memory_grant_list->connect("item_selected", Callable(this, "_on_grant_item_selected"));
    memory_grant_list->connect("gui_input", Callable(this, "_on_grant_list_gui_input"));
    memory_grant_list->connect("mouse_exited", Callable(this, "_on_grant_list_mouse_exited"));

    // Column B: Primary Visualization View
    view_container = memnew(PanelContainer);
    view_container->set_h_size_flags(SIZE_EXPAND_FILL);
    view_container->set_v_size_flags(SIZE_EXPAND_FILL);
    main_workspace->add_child(view_container);

    memory_buffer_view = memnew(MemoryBufferView);
    view_container->add_child(memory_buffer_view);

    managed_buffer_view = memnew(ManagedBufferView);
    managed_buffer_view->set_visible(false); 
    view_container->add_child(managed_buffer_view);

    memory_grant_view = memnew(MemoryGrantView);
    memory_grant_view->set_visible(false);
    view_container->add_child(memory_grant_view);

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

#ifdef TOOLS_ENABLED
void MemoryProfiler::set_editor_undo_redo(EditorUndoRedoManager* p_manager) {
    if (undo_redo.is_valid()) {
        undo_redo->set_editor_undo_redo(p_manager);
    }
}
#endif

void MemoryProfiler::_on_theme_applied(const Ref<Theme>& p_theme, int p_index) {
    if (p_theme.is_valid()) {
        set_theme(p_theme);
    } else {
        set_theme(nullptr); 
    }
}

void MemoryProfiler::_on_save_pressed() {
    if (active_resource.is_null()) return;

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
        emit_signal("runtime_save_requested", active_resource);
    }
}

void MemoryProfiler::_on_buffer_item_selected(int p_index) {
    if (active_resource.is_null()) return;

    managed_buffer_list->deselect_all();
    memory_grant_list->deselect_all();
    
    managed_buffer_view->set_visible(false);
    memory_grant_view->set_visible(false);
    memory_buffer_view->set_visible(true);
    
    memory_ribbon->clear_dimming();

    TypedArray<MemoryBufferResource> schemas = active_resource->get_buffer_schemas();
    if (p_index >= 0 && p_index < schemas.size()) {
        Ref<MemoryBufferResource> schema = schemas[p_index];
        if (schema.is_valid()) {
            memory_buffer_view->open_buffer(schema);
            inspector_title->set_text(String("Inspecting: ") + schema->get_buffer_name());
        }
    }
}

void MemoryProfiler::_on_managed_buffer_item_selected(int p_index) {
    if (active_resource.is_null()) return;

    memory_buffer_list->deselect_all();
    memory_grant_list->deselect_all();
    
    memory_buffer_view->set_visible(false);
    memory_grant_view->set_visible(false);
    managed_buffer_view->set_visible(true);
    
    memory_ribbon->clear_dimming();

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

void MemoryProfiler::_on_grant_item_selected(int p_index) {
    if (active_resource.is_null()) return;

    memory_buffer_list->deselect_all();
    managed_buffer_list->deselect_all();
    
    memory_buffer_view->set_visible(false);
    managed_buffer_view->set_visible(false);
    memory_grant_view->set_visible(true);

    TypedArray<MemoryGrantResource> grants = active_resource->get_active_emulated_grants();
    if (p_index >= 0 && p_index < grants.size()) {
        Ref<MemoryGrantResource> grant = grants[p_index];
        if (grant.is_valid()) {
            godot::Ref<MemoryGrantInspector> inspector = active_resource->get_grant_inspector(p_index);
            memory_grant_view->populate_from_inspector(inspector); 
            
            inspector_title->set_text(String("Inspecting: ") + grant->get_grant_name());
            
            PackedInt32Array ids = grant->get_buffer_ids(); 
            memory_ribbon->highlight_grant_buffers(ids);
        }
    }
}

void MemoryProfiler::_on_grant_list_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        int item_idx = memory_grant_list->get_item_at_position(mm->get_position(), true);
        if (item_idx != -1 && item_idx != hovered_grant_index) {
            hovered_grant_index = item_idx;
            
            if (active_resource.is_valid()) {
                TypedArray<MemoryGrantResource> grants = active_resource->get_active_emulated_grants();
                if (item_idx < grants.size()) {
                    Ref<MemoryGrantResource> grant = grants[item_idx];
                    if (grant.is_valid()) {
                        PackedInt32Array ids = grant->get_buffer_ids();
                        memory_ribbon->highlight_grant_buffers(ids);
                    }
                }
            }
        } else if (item_idx == -1 && hovered_grant_index != -1) {
            hovered_grant_index = -1;
            memory_ribbon->clear_dimming();
        }
    }
}

void MemoryProfiler::_on_grant_list_mouse_exited() {
    hovered_grant_index = -1;
    memory_ribbon->clear_dimming();
}

void MemoryProfiler::_on_ribbon_inspection_requested(int p_block_type, int p_index) {
    // Forward the visual selection from the memory ribbon to our main workspace logic
    if (p_block_type == MemoryRibbon::BLOCK_BUFFER) {
        sidebar_tabs->set_current_tab(0);
        if (memory_buffer_list->get_item_count() > p_index) {
            memory_buffer_list->select(p_index);
            _on_buffer_item_selected(p_index);
        }
    } else if (p_block_type == MemoryRibbon::BLOCK_MANAGED) {
        sidebar_tabs->set_current_tab(1);
        if (managed_buffer_list->get_item_count() > p_index) {
            managed_buffer_list->select(p_index);
            _on_managed_buffer_item_selected(p_index);
        }
    } else if (p_block_type == MemoryRibbon::BLOCK_TRANSIENT) {
        inspector_title->set_text("Inspecting: Transient Capacity");
        memory_buffer_view->open_buffer(Ref<MemoryBufferResource>());
        
        memory_buffer_view->set_visible(false);
        managed_buffer_view->set_visible(false);
        memory_grant_view->set_visible(false);
    }
}

void MemoryProfiler::_populate_ui() {
    memory_buffer_list->clear();
    managed_buffer_list->clear();
    memory_grant_list->clear();

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

    TypedArray<MemoryGrantResource> grants = active_resource->get_active_emulated_grants();
    for (int i = 0; i < grants.size(); ++i) {
        Ref<MemoryGrantResource> grant = grants[i];
        if (grant.is_valid()) {
            memory_grant_list->add_item(String("Grant Profile ") + String::num_int64(i));
        } else {
            memory_grant_list->add_item("Invalid Grant");
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

    memory_grant_list->clear();
    memory_grant_view->clear();

    if (memory_ribbon) {
        memory_ribbon->sync_with_resource(Ref<MemoryManagerResource>());
    }
}

Window* MemoryProfiler::create_profiler_window() {
    Window* profiler_window = memnew(Window);
    profiler_window->set_title("Memory Profiler");
    profiler_window->set_min_size(Vector2i(900, 600)); 
    profiler_window->set_transient(false);
    
    profiler_window->connect("close_requested", Callable(profiler_window, "queue_free"));

    SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (tree && tree->get_root()) {
        tree->get_root()->add_child(profiler_window);
    }
    return profiler_window;
}

#ifdef TOOLS_ENABLED
void MemoryProfiler::profile_memory_manager(Ref<MemoryManagerResource> p_resource, Control* p_owner, EditorUndoRedoManager* p_undo_redo) {
#else
void MemoryProfiler::profile_memory_manager(Ref<MemoryManagerResource> p_resource, Control* p_owner) {
#endif
    if (p_resource.is_null()) return;
    
    MemoryProfiler* target_profiler = nullptr;

    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_profiler = Object::cast_to<MemoryProfiler>(p_owner->get_child(i));
            if (target_profiler) break;
        }

        if (!target_profiler) {
            target_profiler = memnew(MemoryProfiler);
            p_owner->add_child(target_profiler);
        }
    } else {
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

#ifdef TOOLS_ENABLED
    if (p_undo_redo && target_profiler) {
        target_profiler->set_editor_undo_redo(p_undo_redo);
    }
#endif
    if (target_profiler) {
        target_profiler->open_resource(p_resource);
    }
}

void MemoryProfiler::close_memory_profiler(Ref<MemoryManagerResource> p_resource, Control* p_owner) {
    MemoryProfiler* target_profiler = nullptr;

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

    if (target_profiler && target_profiler->active_resource == p_resource) {
        target_profiler->close_resource();
    }
}

} // namespace ideam::godot_ext