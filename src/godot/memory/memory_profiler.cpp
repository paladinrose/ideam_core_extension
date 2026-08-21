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
    ClassDB::bind_method(D_METHOD("_on_buffer_list_gui_input", "event"), &MemoryProfiler::_on_buffer_list_gui_input);
    ClassDB::bind_method(D_METHOD("_update_buffer_selection", "selection"), &MemoryProfiler::_update_buffer_selection);
    ClassDB::bind_method(D_METHOD("_select_all_buffers"), &MemoryProfiler::_select_all_buffers);
    ClassDB::bind_method(D_METHOD("_invert_buffer_selection"), &MemoryProfiler::_invert_buffer_selection);
    ClassDB::bind_method(D_METHOD("_buffer_cut"), &MemoryProfiler::_buffer_cut);
    ClassDB::bind_method(D_METHOD("_buffer_copy"), &MemoryProfiler::_buffer_copy);
    ClassDB::bind_method(D_METHOD("_buffer_paste"), &MemoryProfiler::_buffer_paste);
    ClassDB::bind_method(D_METHOD("_clear_clipboard"), &MemoryProfiler::_clear_clipboard);

    ClassDB::bind_method(D_METHOD("_on_managed_buffer_item_selected", "index"), &MemoryProfiler::_on_managed_buffer_item_selected);
    ClassDB::bind_method(D_METHOD("_on_grant_item_selected", "index"), &MemoryProfiler::_on_grant_item_selected);
    
    ClassDB::bind_method(D_METHOD("_on_grant_list_gui_input", "event"), &MemoryProfiler::_on_grant_list_gui_input);
    ClassDB::bind_method(D_METHOD("_on_grant_list_mouse_exited"), &MemoryProfiler::_on_grant_list_mouse_exited);

    ClassDB::bind_method(D_METHOD("_on_ribbon_inspection_requested", "block_type", "index", "shift_pressed", "ctrl_pressed"), &MemoryProfiler::_on_ribbon_inspection_requested);
    
    ClassDB::bind_method(D_METHOD("_on_theme_applied", "theme", "index"), &MemoryProfiler::_on_theme_applied);
    
    ClassDB::bind_method(D_METHOD("_on_part_selection_inspection_requested", "part_index"), &MemoryProfiler::_on_part_selection_inspection_requested);

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
    memory_ribbon->connect("select_all_requested", Callable(this, "_select_all_buffers"));
    memory_ribbon->connect("invert_selection_requested", Callable(this, "_invert_buffer_selection"));
    memory_ribbon->connect("copy_requested", Callable(this, "_buffer_copy"));
    memory_ribbon->connect("cut_requested", Callable(this, "_buffer_cut"));
    memory_ribbon->connect("paste_requested", Callable(this, "_buffer_paste"));
    memory_ribbon->connect("cancel_requested", Callable(this, "_clear_clipboard"));

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
    memory_buffer_list->set_select_mode(ItemList::SELECT_MULTI);
    sidebar_tabs->add_child(memory_buffer_list);
    memory_buffer_list->connect("item_selected", Callable(this, "_on_buffer_item_selected"));
    memory_buffer_list->connect("gui_input", Callable(this, "_on_buffer_list_gui_input"));

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
   
    // Connect the new signal
    memory_grant_view->connect("part_selection_inspection_requested", Callable(this, "_on_part_selection_inspection_requested"));
    
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
    last_clicked_buffer_index = p_index; // Track the anchor
    PackedInt32Array new_selection = memory_buffer_list->get_selected_items();
    _update_buffer_selection(new_selection);
}

void MemoryProfiler::_on_buffer_list_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventKey> k = p_event;
    if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
        
        // Escape cancels clipboard operations
        if (k->get_keycode() == Key::KEY_ESCAPE) {
            _clear_clipboard();
            memory_buffer_list->accept_event();
            return;
        }

        if (k->is_command_or_control_pressed()) {
            if (k->get_keycode() == Key::KEY_I) {
                _invert_buffer_selection();
                memory_buffer_list->accept_event();
                return;
            } else if (k->get_keycode() == Key::KEY_C) {
                _buffer_copy();
                memory_buffer_list->accept_event();
                return;
            } else if (k->get_keycode() == Key::KEY_X) {
                _buffer_cut();
                memory_buffer_list->accept_event();
                return;
            } else if (k->get_keycode() == Key::KEY_V) {
                _buffer_paste();
                memory_buffer_list->accept_event();
                return;
            }
        }
    }
}

void MemoryProfiler::_update_buffer_selection(const PackedInt32Array& p_selection) {
    if (active_resource.is_null()) return;

    selected_buffer_ids = p_selection;

    // 1. Sync the Ribbon
    memory_ribbon->set_selected_buffers(selected_buffer_ids);

    // 2. Sync the ItemList silently
    memory_buffer_list->deselect_all();
    for (int i = 0; i < selected_buffer_ids.size(); ++i) {
        // The 'false' argument prevents the ItemList from emitting signals
        memory_buffer_list->select(selected_buffer_ids[i], false); 
    }

    // 3. Manage the Views
    managed_buffer_list->deselect_all();
    memory_grant_list->deselect_all();
    managed_buffer_view->set_visible(false);
    memory_grant_view->set_visible(false);
    memory_buffer_view->set_visible(true);

    // 4. Update the Inspector Panel
    if (selected_buffer_ids.size() == 1) {
        // Single selection: Show normal buffer inspection
        TypedArray<MemoryBufferResource> schemas = active_resource->get_buffer_schemas();
        int idx = selected_buffer_ids[0];
        if (idx >= 0 && idx < schemas.size()) {
            Ref<MemoryBufferResource> schema = schemas[idx];
            if (schema.is_valid()) {
                TypedArray<MemoryBufferResource> view_array;
                view_array.append(schema);
                memory_buffer_view->open_buffers(view_array);
                inspector_title->set_text(String("Inspecting: ") + schema->get_buffer_name());
            }
        }
    } else if (selected_buffer_ids.size() > 1) {
        // Multi-selection : TO BE UPDATED!!
        memory_buffer_view->open_buffers(TypedArray<MemoryBufferResource>());
        inspector_title->set_text(String("Multiple Buffers Selected (") + String::num_int64(selected_buffer_ids.size()) + ")");
    } else {
        // Empty selection
        memory_buffer_view->open_buffers(TypedArray<MemoryBufferResource>());
        inspector_title->set_text("Inspector");
    }
}

void MemoryProfiler::_select_all_buffers() {
    if (active_resource.is_null()) return;

    int count = active_resource->get_buffer_schemas().size();
    PackedInt32Array new_selection;
    
    for (int i = 0; i < count; ++i) {
        new_selection.append(i);
    }
    
    _update_buffer_selection(new_selection);
}

void MemoryProfiler::_invert_buffer_selection() {
    if (active_resource.is_null()) return;

    int count = active_resource->get_buffer_schemas().size();
    PackedInt32Array new_selection;
    
    for (int i = 0; i < count; ++i) {
        if (!selected_buffer_ids.has(i)) {
            new_selection.append(i);
        }
    }
    
    _update_buffer_selection(new_selection);
}

void MemoryProfiler::_buffer_copy() {
    if (active_resource.is_null() || selected_buffer_ids.is_empty()) return;
    
    clipboard_buffer_ids = selected_buffer_ids;
    is_cut_operation = false;
    
    // Clear any previous cut visuals
    memory_ribbon->clear_cut_buffers();
}

void MemoryProfiler::_buffer_cut() {
    if (active_resource.is_null() || selected_buffer_ids.is_empty()) return;
    
    clipboard_buffer_ids = selected_buffer_ids;
    is_cut_operation = true;
    
    // Visually stage the cut in the UI
    memory_ribbon->set_cut_buffers(clipboard_buffer_ids);
}

void MemoryProfiler::_buffer_paste() {
    if (active_resource.is_null() || clipboard_buffer_ids.is_empty()) return;

    // Use our anchor index as the target
    int target_index = last_clicked_buffer_index;

    if (is_cut_operation) {
        // Assume this method will handle the complex gathering/reordering math 
        active_resource->move_buffers_bulk(clipboard_buffer_ids, target_index);
        
        // A cut is a one-time operation. Clear the clipboard after pasting.
        _clear_clipboard();
    } else {
        // Assume this method handles bulk duplication
        active_resource->duplicate_buffers_bulk(clipboard_buffer_ids, target_index);
    }
}

void MemoryProfiler::_clear_clipboard() {
    clipboard_buffer_ids.clear();
    is_cut_operation = false;
    memory_ribbon->clear_cut_buffers();
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

void MemoryProfiler::_on_ribbon_inspection_requested(int p_block_type, int p_index, bool p_shift_pressed, bool p_ctrl_pressed) {
    if (p_block_type == MemoryRibbon::BLOCK_BUFFER) {
        sidebar_tabs->set_current_tab(0);

        PackedInt32Array new_selection;

        if (p_shift_pressed && last_clicked_buffer_index != -1) {
            // RANGE SELECTION
            if (p_ctrl_pressed) {
                new_selection = selected_buffer_ids; // Keep existing if Ctrl is held
            }
            int start = (last_clicked_buffer_index < p_index) ? last_clicked_buffer_index : p_index;
            int end = (last_clicked_buffer_index > p_index) ? last_clicked_buffer_index : p_index;
            
            for (int i = start; i <= end; ++i) {
                if (!new_selection.has(i)) {
                    new_selection.append(i);
                }
            }
        } else if (p_ctrl_pressed) {
            // TOGGLE SELECTION
            new_selection = selected_buffer_ids;
            if (new_selection.has(p_index)) {
                // Strip the index out
                PackedInt32Array temp;
                for (int i = 0; i < new_selection.size(); ++i) {
                    if (new_selection[i] != p_index) {
                        temp.append(new_selection[i]);
                    }
                }
                new_selection = temp;
            } else {
                new_selection.append(p_index);
            }
            last_clicked_buffer_index = p_index;
        } else {
            // STANDARD SELECTION
            new_selection.append(p_index);
            last_clicked_buffer_index = p_index;
        }

        // Push to the hub
        _update_buffer_selection(new_selection);

    } else if (p_block_type == MemoryRibbon::BLOCK_MANAGED) {
        sidebar_tabs->set_current_tab(1);
        if (managed_buffer_list->get_item_count() > p_index) {
            managed_buffer_list->select(p_index);
            _on_managed_buffer_item_selected(p_index);
        }
    } else if (p_block_type == MemoryRibbon::BLOCK_TRANSIENT) {
        inspector_title->set_text("Inspecting: Transient Capacity");
        memory_buffer_view->open_buffers(TypedArray<MemoryBufferResource>());
        
        memory_buffer_view->set_visible(false);
        managed_buffer_view->set_visible(false);
        memory_grant_view->set_visible(false);
    }
}

void MemoryProfiler::_on_part_selection_inspection_requested(int p_part_index) {
    // 1. Clear out the placeholder or old views (keep the title)
    for (int i = inspector_content->get_child_count() - 1; i >= 0; --i) {
        Node* child = inspector_content->get_child(i);
        if (child != inspector_title) {
            inspector_content->remove_child(child);
            child->queue_free();
        }
    }

    // 2. Instantiate and add the Selection View
    current_selection_view = memnew(MemoryBufferSelectionView);
    inspector_content->add_child(current_selection_view);
    inspector_title->set_text(String("Inspecting: Grant Part ") + String::num_int64(p_part_index));

    // 3. Fetch the specific MemorySelectionInspector and populate
    // Note: Adjust the getter below to match whatever your actual 
    // getter is on the active_resource or memory_grant_inspector.
    
    int selected_grant_idx = memory_grant_list->get_selected_items()[0];
    Ref<MemoryGrantInspector> grant_inspector = active_resource->get_grant_inspector(selected_grant_idx);
    
    Ref<MemorySelectionInspector> selection_inspector = grant_inspector->get_part_snapshot(p_part_index)["selection"];
    
    if (selection_inspector.is_valid()) {
        current_selection_view->populate(selection_inspector);
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
    memory_buffer_view->open_buffers(TypedArray<MemoryBufferResource>());;
    
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