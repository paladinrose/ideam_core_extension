#include "memory_profiler.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

using ideam::core::BufferLayoutType;

void MemoryProfiler::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_save_pressed"), &MemoryProfiler::_on_save_pressed);
    
    ClassDB::bind_method(D_METHOD("_show_context_menu", "global_pos", "buffer_id"), &MemoryProfiler::_show_context_menu);
    ClassDB::bind_method(D_METHOD("_on_context_menu_id_pressed", "id"), &MemoryProfiler::_on_context_menu_id_pressed);

    ClassDB::bind_method(D_METHOD("_on_buffer_item_selected", "index"), &MemoryProfiler::_on_buffer_item_selected);
    ClassDB::bind_method(D_METHOD("_on_buffer_list_gui_input", "event"), &MemoryProfiler::_on_buffer_list_gui_input);
    ClassDB::bind_method(D_METHOD("_update_buffer_selection", "selection"), &MemoryProfiler::_update_buffer_selection);
    ClassDB::bind_method(D_METHOD("_select_all_buffers"), &MemoryProfiler::_select_all_buffers);
    ClassDB::bind_method(D_METHOD("_invert_buffer_selection"), &MemoryProfiler::_invert_buffer_selection);
    ClassDB::bind_method(D_METHOD("_buffer_cut"), &MemoryProfiler::_buffer_cut);
    ClassDB::bind_method(D_METHOD("_buffer_copy"), &MemoryProfiler::_buffer_copy);
    ClassDB::bind_method(D_METHOD("_buffer_paste"), &MemoryProfiler::_buffer_paste);
    ClassDB::bind_method(D_METHOD("_request_delete_selection", "bypass_confirm"), &MemoryProfiler::_request_delete_selection);
    ClassDB::bind_method(D_METHOD("_clear_clipboard"), &MemoryProfiler::_clear_clipboard);
    ClassDB::bind_method(D_METHOD("_copy_resource_path"), &MemoryProfiler::_copy_resource_path);
    ClassDB::bind_method(D_METHOD("_copy_resource_uid"), &MemoryProfiler::_copy_resource_uid);

    ClassDB::bind_method(D_METHOD("_on_buffer_list_get_drag_data", "at_position"), &MemoryProfiler::_on_buffer_list_get_drag_data);
    ClassDB::bind_method(D_METHOD("_on_buffer_list_can_drop_data", "at_position", "data"), &MemoryProfiler::_on_buffer_list_can_drop_data);
    ClassDB::bind_method(D_METHOD("_on_buffer_list_drop_data", "at_position", "data"), &MemoryProfiler::_on_buffer_list_drop_data);

    ClassDB::bind_method(D_METHOD("_on_managed_list_get_drag_data", "at_position"), &MemoryProfiler::_on_managed_list_get_drag_data);
    ClassDB::bind_method(D_METHOD("_on_managed_list_can_drop_data", "at_position", "data"), &MemoryProfiler::_on_managed_list_can_drop_data);
    ClassDB::bind_method(D_METHOD("_on_managed_list_drop_data", "at_position", "data"), &MemoryProfiler::_on_managed_list_drop_data);

    ClassDB::bind_method(D_METHOD("_on_managed_buffer_item_selected", "index"), &MemoryProfiler::_on_managed_buffer_item_selected);
    ClassDB::bind_method(D_METHOD("_on_grant_item_selected", "index"), &MemoryProfiler::_on_grant_item_selected);
    
    ClassDB::bind_method(D_METHOD("_on_grant_list_gui_input", "event"), &MemoryProfiler::_on_grant_list_gui_input);
    ClassDB::bind_method(D_METHOD("_on_grant_list_mouse_exited"), &MemoryProfiler::_on_grant_list_mouse_exited);

    ClassDB::bind_method(D_METHOD("_on_ribbon_inspection_requested", "block_type", "index", "shift_pressed", "ctrl_pressed"), &MemoryProfiler::_on_ribbon_inspection_requested);
    ClassDB::bind_method(D_METHOD("_on_theme_applied", "theme", "index"), &MemoryProfiler::_on_theme_applied);
    ClassDB::bind_method(D_METHOD("_on_part_selection_inspection_requested", "part_index"), &MemoryProfiler::_on_part_selection_inspection_requested);
    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &MemoryProfiler::_on_resource_changed);

    ClassDB::bind_method(D_METHOD("_on_ribbon_drag_initiated", "origin_buffer_id"), &MemoryProfiler::_on_ribbon_drag_initiated);
    ClassDB::bind_method(D_METHOD("_on_ribbon_drop_paste_requested", "target_index"), &MemoryProfiler::_on_ribbon_drop_paste_requested);
    ClassDB::bind_method(D_METHOD("_on_delete_confirmed"), &MemoryProfiler::_on_delete_confirmed);

    ADD_SIGNAL(MethodInfo("runtime_delete_requested", PropertyInfo(Variant::PACKED_INT32_ARRAY, "buffer_ids")));
    ADD_SIGNAL(MethodInfo("runtime_save_requested", PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource")));
}

MemoryProfiler::MemoryProfiler() {
    undo_redo.instantiate();
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_process_input(true);

    // --- Context Menu Setup ---
    context_menu = memnew(PopupMenu);
    add_child(context_menu);
    context_menu->connect("id_pressed", Callable(this, "_on_context_menu_id_pressed"));

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

    // Confirmation dialog for delete operations
    delete_confirm_dialog = memnew(ConfirmationDialog);
    delete_confirm_dialog->set_title("Confirm Buffer Deletion");
    delete_confirm_dialog->set_text(
        "WARNING! This will fully delete the selected buffer(s)! "
        "If other resources are making use of them, this will sever those connections! "
        "Are you sure you want to proceed?"
    );
    delete_confirm_dialog->set_ok_button_text("Confirm Delete");
    delete_confirm_dialog->set_cancel_button_text("Cancel");
    add_child(delete_confirm_dialog);
    delete_confirm_dialog->connect("confirmed", Callable(this, "_on_delete_confirmed"));

    // --- Row 2 & 3: Vertical Split (Ribbon on top, Workspace on bottom) ---
    ribbon_workspace_split = memnew(VSplitContainer);
    ribbon_workspace_split->set_h_size_flags(SIZE_EXPAND_FILL);
    ribbon_workspace_split->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(ribbon_workspace_split);

    // --- Upper Pane: Memory Ribbon Container ---
    ribbon_container = memnew(PanelContainer);
    ribbon_container->set_h_size_flags(SIZE_EXPAND_FILL);
    ribbon_container->set_custom_minimum_size(Vector2(0, 80));
    ribbon_workspace_split->add_child(ribbon_container);

    memory_ribbon = memnew(MemoryRibbon);
    memory_ribbon->set_h_size_flags(SIZE_EXPAND_FILL);
    memory_ribbon->set_v_size_flags(SIZE_EXPAND_FILL);
    memory_ribbon->set_focus_mode(FOCUS_ALL);
    ribbon_container->add_child(memory_ribbon);
    
    memory_ribbon->connect("inspection_requested", Callable(this, "_on_ribbon_inspection_requested"));
    memory_ribbon->connect("select_all_requested", Callable(this, "_select_all_buffers"));
    memory_ribbon->connect("invert_selection_requested", Callable(this, "_invert_buffer_selection"));
    memory_ribbon->connect("copy_requested", Callable(this, "_buffer_copy"));
    memory_ribbon->connect("cut_requested", Callable(this, "_buffer_cut"));
    memory_ribbon->connect("paste_requested", Callable(this, "_buffer_paste"));
    memory_ribbon->connect("cancel_requested", Callable(this, "_clear_clipboard"));
    memory_ribbon->connect("buffer_context_menu_requested", Callable(this, "_show_context_menu"));
    memory_ribbon->connect("drag_initiated", Callable(this, "_on_ribbon_drag_initiated"));
    memory_ribbon->connect("drop_paste_requested", Callable(this, "_on_ribbon_drop_paste_requested"));
    memory_ribbon->connect("delete_requested", Callable(this, "_request_delete_selection"));

    // --- Row 3: Main Workspace ---
    workspace_split = memnew(HSplitContainer);
    workspace_split->set_h_size_flags(SIZE_EXPAND_FILL);
    workspace_split->set_v_size_flags(SIZE_EXPAND_FILL);
    ribbon_workspace_split->add_child(workspace_split);

    // Column A: Buffer Selection Sidebar (Left Pane)
    sidebar_tabs = memnew(TabContainer);
    sidebar_tabs->set_custom_minimum_size(Vector2(200, 0)); 
    sidebar_tabs->set_v_size_flags(SIZE_EXPAND_FILL);
    sidebar_tabs->set_focus_mode(FOCUS_ALL);
    workspace_split->add_child(sidebar_tabs);

    memory_buffer_list = memnew(ItemList);
    memory_buffer_list->set_name("Buffers");
    memory_buffer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    memory_buffer_list->set_v_size_flags(SIZE_EXPAND_FILL);
    memory_buffer_list->set_select_mode(ItemList::SELECT_MULTI);
    memory_buffer_list->set_focus_mode(FOCUS_ALL);
    sidebar_tabs->add_child(memory_buffer_list);
    memory_buffer_list->connect("item_selected", Callable(this, "_on_buffer_item_selected"));
    memory_buffer_list->connect("gui_input", Callable(this, "_on_buffer_list_gui_input"));

    memory_buffer_list->set_drag_forwarding(
        Callable(this, "_on_buffer_list_get_drag_data"),
        Callable(this, "_on_buffer_list_can_drop_data"),
        Callable(this, "_on_buffer_list_drop_data")
    );

    managed_buffer_list = memnew(ItemList);
    managed_buffer_list->set_name("Managed");
    managed_buffer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    managed_buffer_list->set_v_size_flags(SIZE_EXPAND_FILL);
    managed_buffer_list->set_focus_mode(FOCUS_ALL);
    sidebar_tabs->add_child(managed_buffer_list);
    managed_buffer_list->connect("item_selected", Callable(this, "_on_managed_buffer_item_selected"));

    managed_buffer_list->set_drag_forwarding(
        Callable(this, "_on_managed_list_get_drag_data"),
        Callable(this, "_on_managed_list_can_drop_data"),
        Callable(this, "_on_managed_list_drop_data")
    );

    memory_grant_list = memnew(ItemList);
    memory_grant_list->set_name("Grants");
    memory_grant_list->set_h_size_flags(SIZE_EXPAND_FILL);
    memory_grant_list->set_v_size_flags(SIZE_EXPAND_FILL);
    memory_grant_list->set_focus_mode(FOCUS_ALL);
    sidebar_tabs->add_child(memory_grant_list);
    memory_grant_list->connect("item_selected", Callable(this, "_on_grant_item_selected"));
    memory_grant_list->connect("gui_input", Callable(this, "_on_grant_list_gui_input"));
    memory_grant_list->connect("mouse_exited", Callable(this, "_on_grant_list_mouse_exited"));

    // --- Inner Horizontal Split (Center View vs Inspector) ---
    workspace_inner_split = memnew(HSplitContainer);
    workspace_inner_split->set_h_size_flags(SIZE_EXPAND_FILL);
    workspace_inner_split->set_v_size_flags(SIZE_EXPAND_FILL);
    workspace_split->add_child(workspace_inner_split);

    // Column B: Primary Visualization View (Center Pane)
    center_scroll = memnew(ScrollContainer);
    center_scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    center_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    center_scroll->set_custom_minimum_size(Vector2(200, 0));
    center_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
    center_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
    center_scroll->set_focus_mode(FOCUS_ALL);
    workspace_inner_split->add_child(center_scroll);

    view_container = memnew(PanelContainer);
    view_container->set_h_size_flags(SIZE_EXPAND_FILL);
    view_container->set_v_size_flags(SIZE_EXPAND_FILL);
    view_container->set_custom_minimum_size(Vector2(250, 0));
    center_scroll->add_child(view_container);

    memory_buffer_view = memnew(MemoryBufferView);
    view_container->add_child(memory_buffer_view);

    managed_buffer_view = memnew(ManagedBufferView);
    managed_buffer_view->set_visible(false); 
    view_container->add_child(managed_buffer_view);

    memory_grant_view = memnew(MemoryGrantView);
    memory_grant_view->set_visible(false);
    view_container->add_child(memory_grant_view);
    memory_grant_view->connect("part_selection_inspection_requested", Callable(this, "_on_part_selection_inspection_requested"));
    
    // Column C: Pseudo-Inspector Panel
    inspector_panel = memnew(PanelContainer);
    inspector_panel->set_custom_minimum_size(Vector2(250, 0));
    inspector_panel->set_v_size_flags(SIZE_EXPAND_FILL);
    inspector_panel->set_focus_mode(FOCUS_ALL);
    workspace_inner_split->add_child(inspector_panel);

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

void MemoryProfiler::_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventKey> k = p_event;
    if (!k.is_valid() || !k->is_pressed() || k->is_echo()) {
        return;
    }

    Viewport* vp = get_viewport();
    Control* focus_owner = vp ? vp->gui_get_focus_owner() : nullptr;
    if (!focus_owner || (focus_owner != this && !is_ancestor_of(focus_owner))) {
        return;
    }

    // Tab / Shift+Tab region cycling across Ribbon, Sidebar, Center View, and Inspector
    if (k->get_keycode() == Key::KEY_TAB && !k->is_command_or_control_pressed() && !k->is_alt_pressed()) {
        _cycle_focus(k->is_shift_pressed());
        if (vp) vp->set_input_as_handled();
    }
}

void MemoryProfiler::_cycle_focus(bool p_reverse) {
    Viewport* vp = get_viewport();
    if (!vp) return;

    Control* current_focus = vp->gui_get_focus_owner();

    enum FocusRegion {
        REGION_RIBBON = 0,
        REGION_SIDEBAR = 1,
        REGION_CENTER = 2,
        REGION_INSPECTOR = 3,
        REGION_COUNT = 4
    };

    int current_region = -1;
    if (current_focus) {
        if (current_focus == memory_ribbon || (memory_ribbon && memory_ribbon->is_ancestor_of(current_focus))) {
            current_region = REGION_RIBBON;
        } else if (current_focus == sidebar_tabs || (sidebar_tabs && sidebar_tabs->is_ancestor_of(current_focus))) {
            current_region = REGION_SIDEBAR;
        } else if (current_focus == center_scroll || (center_scroll && center_scroll->is_ancestor_of(current_focus))) {
            current_region = REGION_CENTER;
        } else if (current_focus == inspector_panel || (inspector_panel && inspector_panel->is_ancestor_of(current_focus))) {
            current_region = REGION_INSPECTOR;
        }
    }

    int next_region;
    if (current_region == -1) {
        next_region = REGION_RIBBON;
    } else if (p_reverse) {
        next_region = (current_region - 1 + REGION_COUNT) % REGION_COUNT;
    } else {
        next_region = (current_region + 1) % REGION_COUNT;
    }

    switch (next_region) {
        case REGION_RIBBON: {
            Control* target_block = nullptr;
            if (memory_ribbon) {
                for (int i = 0; i < memory_ribbon->get_child_count(); ++i) {
                    MemoryBlockButton* b = Object::cast_to<MemoryBlockButton>(memory_ribbon->get_child(i));
                    if (b) {
                        if (b->get_selected()) {
                            target_block = b;
                            break;
                        }
                        if (!target_block) {
                            target_block = b;
                        }
                    }
                }
            }
            if (target_block) {
                target_block->grab_focus();
            } else if (memory_ribbon) {
                memory_ribbon->grab_focus();
            }
            break;
        }
        case REGION_SIDEBAR: {
            int tab = sidebar_tabs ? sidebar_tabs->get_current_tab() : 0;
            if (tab == 0 && memory_buffer_list) {
                memory_buffer_list->grab_focus();
            } else if (tab == 1 && managed_buffer_list) {
                managed_buffer_list->grab_focus();
            } else if (tab == 2 && memory_grant_list) {
                memory_grant_list->grab_focus();
            } else if (sidebar_tabs) {
                sidebar_tabs->grab_focus();
            }
            break;
        }
        case REGION_CENTER: {
            if (memory_buffer_view && memory_buffer_view->is_visible_in_tree()) {
                memory_buffer_view->grab_focus();
            } else if (managed_buffer_view && managed_buffer_view->is_visible_in_tree()) {
                managed_buffer_view->grab_focus();
            } else if (memory_grant_view && memory_grant_view->is_visible_in_tree()) {
                memory_grant_view->grab_focus();
            } else if (center_scroll) {
                center_scroll->grab_focus();
            }
            break;
        }
        case REGION_INSPECTOR: {
            if (current_selection_view && current_selection_view->is_visible_in_tree()) {
                current_selection_view->grab_focus();
            } else if (inspector_panel) {
                inspector_panel->grab_focus();
            }
            break;
        }
    }
}

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

void MemoryProfiler::_show_context_menu(const Vector2& p_global_pos, int p_buffer_id) {
    context_menu_target_buffer_id = p_buffer_id;

    if (p_buffer_id >= 0 && !selected_buffer_ids.has(p_buffer_id)) {
        PackedInt32Array new_sel;
        new_sel.append(p_buffer_id);
        _update_buffer_selection(new_sel);
        last_clicked_buffer_index = p_buffer_id;
    }
    
    for (int i = context_menu->get_child_count() - 1; i >= 0; --i) {
        Node* child = context_menu->get_child(i);
        context_menu->remove_child(child);
        child->queue_free();
    }
    context_menu->clear();

    if (p_buffer_id >= 0) {
        PopupMenu* new_buffer_menu = memnew(PopupMenu);
        new_buffer_menu->set_name("NewBufferMenu");
        new_buffer_menu->add_item("Before", ACTION_NEW_BUFFER_BEFORE);
        new_buffer_menu->add_item("After", ACTION_NEW_BUFFER_AFTER);
        new_buffer_menu->connect("id_pressed", Callable(this, "_on_context_menu_id_pressed"));

        context_menu->add_child(new_buffer_menu);
        context_menu->add_submenu_item("New Buffer", "NewBufferMenu");
    } else {
        context_menu->add_item("New Buffer", ACTION_NEW_BUFFER);
    }

    context_menu->add_separator();

    bool has_selection = !selected_buffer_ids.is_empty();

    if (has_selection) {
        context_menu->add_item("Cut", ACTION_CUT);
        context_menu->add_item("Copy", ACTION_COPY);
    }
    
    if (!clipboard_buffer_ids.is_empty()) {
        context_menu->add_item("Paste", ACTION_PASTE);
    }

    if (has_selection) {
        if (selected_buffer_ids.size() == 1) {
            int id = selected_buffer_ids[0];
            TypedArray<MemoryBufferResource> schemas = active_resource->get_buffer_schemas();
            String b_name = "Buffer";
            if (id >= 0 && id < schemas.size()) {
                Ref<MemoryBufferResource> schema = schemas[id];
                if (schema.is_valid()) {
                    b_name = schema->get_buffer_name();
                }
            }
            context_menu->add_item(String("Delete '") + b_name + "'", ACTION_DELETE);
        } else {
            context_menu->add_item("Delete Selection", ACTION_DELETE);
        }
    }

    if (active_resource.is_valid() && !active_resource->get_path().is_empty()) {
        context_menu->add_separator();
        context_menu->add_item("Copy Resource Path", ACTION_COPY_PATH);
        context_menu->add_item("Copy Resource UID", ACTION_COPY_UID);
    }

    if (context_menu->get_item_count() > 0) {
        context_menu->add_separator();
    }

    // Build Sub-sub-menu: "By Type"
    PopupMenu* type_menu = memnew(PopupMenu);
    type_menu->set_name("ByTypeMenu");
    type_menu->add_item("FLAT", ACTION_SELECT_TYPE_FLAT);
    type_menu->add_item("AOS", ACTION_SELECT_TYPE_AOS);
    type_menu->add_item("SOA", ACTION_SELECT_TYPE_SOA);
    type_menu->add_item("SPARSE_SET", ACTION_SELECT_TYPE_SPARSE_SET);
    type_menu->add_item("TILED_SOA", ACTION_SELECT_TYPE_TILED_SOA);
    type_menu->add_item("RING", ACTION_SELECT_TYPE_RING);
    type_menu->add_item("PAGED", ACTION_SELECT_TYPE_PAGED);
    type_menu->add_separator(); 
    type_menu->add_item("Any Linear", ACTION_SELECT_TYPE_ANY_LINEAR);
    type_menu->add_item("Any Parallel", ACTION_SELECT_TYPE_ANY_PARALLEL);
    type_menu->add_item("Any Spatial", ACTION_SELECT_TYPE_ANY_SPATIAL);
    type_menu->connect("id_pressed", Callable(this, "_on_context_menu_id_pressed"));

    // Build Sub-menu: "Select"
    PopupMenu* select_menu = memnew(PopupMenu);
    select_menu->set_name("SelectMenu");
    select_menu->add_item("All", ACTION_SELECT_ALL);
    select_menu->add_item("Invert", ACTION_INVERT_SELECTION);
    select_menu->add_separator();
    select_menu->add_item("Managed Buffers", ACTION_SELECT_MANAGED_BUFFERS);
    select_menu->add_item("Grants", ACTION_SELECT_GRANTS);
    
    select_menu->add_child(type_menu);
    select_menu->add_submenu_item("By Type", "ByTypeMenu");

    // Dynamic Sub-menu: "By Subscriber"
    context_menu_subscribers.clear();
    PopupMenu* subscriber_menu = memnew(PopupMenu);
    subscriber_menu->set_name("BySubscriberMenu");

    if (active_resource.is_valid()) {
        TypedArray<ManagedBufferResource> managed = active_resource->get_managed_buffers();
        for (int i = 0; i < managed.size(); ++i) {
            Ref<ManagedBufferResource> mbr = managed[i];
            if (mbr.is_valid()) {
                StringName sub_name = mbr->get_consumer_name();
                if (!sub_name.is_empty() && context_menu_subscribers.find(sub_name) == -1) {
                    context_menu_subscribers.push_back(sub_name);
                }
            }
        }
    }

    if (!context_menu_subscribers.is_empty()) {
        for (int i = 0; i < context_menu_subscribers.size(); ++i) {
            subscriber_menu->add_item(context_menu_subscribers[i], ACTION_SELECT_SUBSCRIBER_BASE + i);
        }
        subscriber_menu->connect("id_pressed", Callable(this, "_on_context_menu_id_pressed"));
        select_menu->add_child(subscriber_menu);
        select_menu->add_submenu_item("By Subscriber", "BySubscriberMenu");
    } else {
        subscriber_menu->queue_free();
    }

    select_menu->connect("id_pressed", Callable(this, "_on_context_menu_id_pressed"));
    context_menu->add_child(select_menu);
    context_menu->add_submenu_item("Select", "SelectMenu");

    Vector2i popup_pos;
    if (context_menu->is_embedded()) {
        popup_pos = Vector2i(p_global_pos);
    } else {
        popup_pos = DisplayServer::get_singleton()->mouse_get_position();
    }

    context_menu->set_position(popup_pos);
    context_menu->popup();
}

void MemoryProfiler::_on_context_menu_id_pressed(int p_id) {
    if (active_resource.is_valid()) {
        switch (p_id) {
            case ACTION_NEW_BUFFER: {
                int insert_idx = active_resource->get_buffer_schemas().size();
                active_resource->create_new_buffer(-1);
                
                PackedInt32Array new_sel;
                new_sel.append(insert_idx);
                _update_buffer_selection(new_sel);
                return;
            }
            case ACTION_NEW_BUFFER_BEFORE: {
                if (context_menu_target_buffer_id >= 0) {
                    int insert_idx = context_menu_target_buffer_id;
                    active_resource->create_new_buffer(insert_idx);

                    PackedInt32Array new_sel;
                    new_sel.append(insert_idx);
                    _update_buffer_selection(new_sel);
                }
                return;
            }
            case ACTION_NEW_BUFFER_AFTER: {
                if (context_menu_target_buffer_id >= 0) {
                    int insert_idx = context_menu_target_buffer_id + 1;
                    active_resource->create_new_buffer(insert_idx);

                    PackedInt32Array new_sel;
                    new_sel.append(insert_idx);
                    _update_buffer_selection(new_sel);
                }
                return;
            }
            case ACTION_SELECT_MANAGED_BUFFERS: {
                sidebar_tabs->set_current_tab(1);
                if (managed_buffer_list && managed_buffer_list->get_item_count() > 0) {
                    managed_buffer_list->select(0);
                    _on_managed_buffer_item_selected(0);
                }
                return;
            }
            case ACTION_SELECT_GRANTS: {
                sidebar_tabs->set_current_tab(2);
                if (memory_grant_list && memory_grant_list->get_item_count() > 0) {
                    memory_grant_list->select(0);
                    _on_grant_item_selected(0);
                }
                return;
            }
        }
    }

    if (p_id >= ACTION_SELECT_SUBSCRIBER_BASE && p_id < ACTION_SELECT_SUBSCRIBER_BASE + context_menu_subscribers.size()) {
        int sub_idx = p_id - ACTION_SELECT_SUBSCRIBER_BASE;
        StringName selected_sub = context_menu_subscribers[sub_idx];

        PackedInt32Array matching_buffer_ids;
        if (active_resource.is_valid()) {
            TypedArray<ManagedBufferResource> managed = active_resource->get_managed_buffers();
            for (int i = 0; i < managed.size(); ++i) {
                Ref<ManagedBufferResource> mbr = managed[i];
                if (mbr.is_valid() && mbr->get_consumer_name() == selected_sub) {
                    int b_id = mbr->resolve_buffer_id();
                    if (b_id >= 0 && !matching_buffer_ids.has(b_id)) {
                        matching_buffer_ids.append(b_id);
                    }
                }
            }
        }

        if (!matching_buffer_ids.is_empty()) {
            sidebar_tabs->set_current_tab(0);
            _update_buffer_selection(matching_buffer_ids);
        } else {
            sidebar_tabs->set_current_tab(1);
            if (active_resource.is_valid()) {
                TypedArray<ManagedBufferResource> managed = active_resource->get_managed_buffers();
                for (int i = 0; i < managed.size(); ++i) {
                    Ref<ManagedBufferResource> mbr = managed[i];
                    if (mbr.is_valid() && mbr->get_consumer_name() == selected_sub) {
                        managed_buffer_list->select(i);
                        _on_managed_buffer_item_selected(i);
                        break;
                    }
                }
            }
        }
        return;
    }

    switch (p_id) {
        case ACTION_CUT: _buffer_cut(); return;
        case ACTION_COPY: _buffer_copy(); return;
        case ACTION_PASTE: _buffer_paste(); return;
        case ACTION_DELETE: _request_delete_selection(false); return;
        case ACTION_SELECT_ALL: _select_all_buffers(); return;
        case ACTION_INVERT_SELECTION: _invert_buffer_selection(); return;
        case ACTION_COPY_PATH: _copy_resource_path(); return;
        case ACTION_COPY_UID: _copy_resource_uid(); return;
    }

    if (active_resource.is_valid()) {
        PackedInt32Array new_selection;
        switch (p_id) {
            case ACTION_SELECT_TYPE_FLAT: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::FLAT)); break;
            case ACTION_SELECT_TYPE_AOS: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::AOS)); break;
            case ACTION_SELECT_TYPE_SOA: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::SOA)); break;
            case ACTION_SELECT_TYPE_SPARSE_SET: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::SPARSE_SET)); break;
            case ACTION_SELECT_TYPE_TILED_SOA: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::TILED_SOA)); break;
            case ACTION_SELECT_TYPE_RING: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::RING)); break;
            case ACTION_SELECT_TYPE_PAGED: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::PAGED)); break;
            case ACTION_SELECT_TYPE_ANY_LINEAR: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::ANY_LINEAR)); break;
            case ACTION_SELECT_TYPE_ANY_PARALLEL: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::ANY_PARALLEL)); break;
            case ACTION_SELECT_TYPE_ANY_SPATIAL: 
                new_selection = active_resource->_select_buffers_by_type(static_cast<int>(ideam::core::BufferLayoutType::ANY_SPATIAL)); break;
        }
        
        if (!new_selection.is_empty()) {
            _update_buffer_selection(new_selection);
        }
    }
}

Variant MemoryProfiler::_on_buffer_list_get_drag_data(const Vector2& p_at_position) {
    if (!active_resource.is_valid()) return Variant();

    int item_idx = memory_buffer_list->get_item_at_position(p_at_position, true);
    if (item_idx < 0) return Variant();

    if (!selected_buffer_ids.has(item_idx)) {
        PackedInt32Array new_sel;
        new_sel.append(item_idx);
        _update_buffer_selection(new_sel);
    }

    _buffer_cut();

    Dictionary drag_payload;
    drag_payload["type"] = "memory_buffer_reorder";
    drag_payload["origin_buffer_id"] = item_idx;

    PanelContainer* preview_panel = memnew(PanelContainer);
    Label* preview_label = memnew(Label);
    preview_label->set_text(memory_buffer_list->get_item_text(item_idx) + "...");
    preview_panel->add_child(preview_label);
    memory_buffer_list->set_drag_preview(preview_panel);

    return drag_payload;
}

bool MemoryProfiler::_on_buffer_list_can_drop_data(const Vector2& p_at_position, const Variant& p_data) const {
    if (p_data.get_type() != Variant::DICTIONARY) return false;
    Dictionary d = p_data;
    return d.has("type") && String(d["type"]) == "memory_buffer_reorder";
}

void MemoryProfiler::_on_buffer_list_drop_data(const Vector2& p_at_position, const Variant& p_data) {
    if (!active_resource.is_valid() || !_on_buffer_list_can_drop_data(p_at_position, p_data)) return;
    int target_idx = memory_buffer_list->get_item_at_position(p_at_position, true);
    if (target_idx < 0) {
        target_idx = memory_buffer_list->get_item_count() - 1;
    }
    if (target_idx < 0) target_idx = 0;

    last_clicked_buffer_index = target_idx;
    _buffer_paste();
}

Variant MemoryProfiler::_on_managed_list_get_drag_data(const Vector2& p_at_position) {
    if (!active_resource.is_valid()) return Variant();

    int item_idx = managed_buffer_list->get_item_at_position(p_at_position, true);
    if (item_idx < 0) return Variant();

    Dictionary drag_payload;
    drag_payload["type"] = "managed_buffer_reorder";
    drag_payload["origin_index"] = item_idx;

    PanelContainer* preview_panel = memnew(PanelContainer);
    Label* preview_label = memnew(Label);
    preview_label->set_text(managed_buffer_list->get_item_text(item_idx) + "...");
    preview_panel->add_child(preview_label);
    managed_buffer_list->set_drag_preview(preview_panel);

    return drag_payload;
}

bool MemoryProfiler::_on_managed_list_can_drop_data(const Vector2& p_at_position, const Variant& p_data) const {
    if (p_data.get_type() != Variant::DICTIONARY) return false;
    Dictionary d = p_data;
    return d.has("type") && String(d["type"]) == "managed_buffer_reorder";
}

void MemoryProfiler::_on_managed_list_drop_data(const Vector2& p_at_position, const Variant& p_data) {
    if (!active_resource.is_valid() || !_on_managed_list_can_drop_data(p_at_position, p_data)) return;
    Dictionary d = p_data;
    int origin_idx = d.get("origin_index", -1);
    int target_idx = managed_buffer_list->get_item_at_position(p_at_position, true);
    if (target_idx < 0) {
        target_idx = managed_buffer_list->get_item_count() - 1;
    }
    if (target_idx < 0) target_idx = 0;

    if (origin_idx >= 0 && origin_idx != target_idx) {
        if (undo_redo.is_valid()) {
            TypedArray<ManagedBufferResource> old_managed = active_resource->get_managed_buffers();
            TypedArray<ManagedBufferResource> new_managed = old_managed.duplicate();
            Variant item = new_managed[origin_idx];
            new_managed.remove_at(origin_idx);
            new_managed.insert(target_idx, item);

            undo_redo->create_action("Move Managed Profile");
            undo_redo->add_do_method(Callable(active_resource.ptr(), "set_managed_buffers").bind(new_managed));
            undo_redo->add_undo_method(Callable(active_resource.ptr(), "set_managed_buffers").bind(old_managed.duplicate()));
            undo_redo->commit_action();
        } else {
            active_resource->move_managed_buffer(origin_idx, target_idx);
        }
        managed_buffer_list->select(target_idx);
        _on_managed_buffer_item_selected(target_idx);
    }
}

void MemoryProfiler::_on_buffer_item_selected(int p_index) {
    last_clicked_buffer_index = p_index;
    PackedInt32Array new_selection = memory_buffer_list->get_selected_items();
    _update_buffer_selection(new_selection);
}

void MemoryProfiler::_on_buffer_list_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed()) {
        if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT) {
            int item_idx = memory_buffer_list->get_item_at_position(mb->get_position(), true);
            if (item_idx != -1 && !memory_buffer_list->is_selected(item_idx)) {
                _on_buffer_item_selected(item_idx); 
            }
            _show_context_menu(mb->get_global_position(), item_idx);
            memory_buffer_list->accept_event();
            return;
        }
    }

    Ref<InputEventKey> k = p_event;
    if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
        if (k->get_keycode() == Key::KEY_ESCAPE) {
            _clear_clipboard();
            memory_buffer_list->accept_event();
            return;
        }
        
        if (k->get_keycode() == Key::KEY_DELETE) {
            _request_delete_selection(k->is_shift_pressed());
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
        memory_buffer_view->open_buffers(TypedArray<MemoryBufferResource>());
        inspector_title->set_text(String("Multiple Buffers Selected (") + String::num_int64(selected_buffer_ids.size()) + ")");
    } else {
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
    memory_ribbon->clear_cut_buffers();
}

void MemoryProfiler::_buffer_cut() {
    if (active_resource.is_null() || selected_buffer_ids.is_empty()) return;
    
    clipboard_buffer_ids = selected_buffer_ids;
    is_cut_operation = true;
    memory_ribbon->set_cut_buffers(clipboard_buffer_ids);
}

void MemoryProfiler::_buffer_paste() {
    if (active_resource.is_null() || clipboard_buffer_ids.is_empty()) return;

    int target_index = last_clicked_buffer_index;

    if (is_cut_operation) {
        if (undo_redo.is_valid()) {
            TypedArray<MemoryBufferResource> old_schemas = active_resource->get_buffer_schemas();
            TypedArray<MemoryBufferResource> remaining;
            TypedArray<MemoryBufferResource> extracted;
            int insert_point = 0;

            for (int i = 0; i < old_schemas.size(); ++i) {
                if (clipboard_buffer_ids.has(i)) {
                    extracted.push_back(old_schemas[i]);
                } else {
                    remaining.push_back(old_schemas[i]);
                    if (i <= target_index) {
                        insert_point++;
                    }
                }
            }

            TypedArray<MemoryBufferResource> new_schemas;
            for (int i = 0; i < insert_point; ++i) {
                new_schemas.push_back(remaining[i]);
            }
            for (int i = 0; i < extracted.size(); ++i) {
                new_schemas.push_back(extracted[i]);
            }
            for (int i = insert_point; i < remaining.size(); ++i) {
                new_schemas.push_back(remaining[i]);
            }

            undo_redo->create_action("Move Buffers (Bulk)");
            undo_redo->add_do_method(Callable(active_resource.ptr(), "set_buffer_schemas").bind(new_schemas));
            undo_redo->add_undo_method(Callable(active_resource.ptr(), "set_buffer_schemas").bind(old_schemas.duplicate()));
            undo_redo->commit_action();
        } else {
            active_resource->move_buffers_bulk(clipboard_buffer_ids, target_index);
        }
        _clear_clipboard();
    } else {
        if (undo_redo.is_valid()) {
            TypedArray<MemoryBufferResource> old_schemas = active_resource->get_buffer_schemas();
            TypedArray<MemoryBufferResource> new_schemas;
            
            int safe_target = target_index;
            if (safe_target < 0) safe_target = -1;
            if (safe_target >= old_schemas.size()) safe_target = old_schemas.size() - 1;

            for (int i = 0; i <= safe_target; ++i) {
                new_schemas.push_back(old_schemas[i]);
            }

            for (int i = 0; i < old_schemas.size(); ++i) {
                if (clipboard_buffer_ids.has(i)) {
                    Ref<MemoryBufferResource> original = old_schemas[i];
                    if (!original.is_valid()) continue;

                    Ref<MemoryBufferResource> duplicate_res;
                    duplicate_res.instantiate();
                    duplicate_res->set_buffer_name(original->get_buffer_name() + StringName("_copy"));
                    duplicate_res->set_layout_type(original->get_layout_type());
                    duplicate_res->set_max_elements(original->get_max_elements());
                    duplicate_res->set_alignment(original->get_alignment());
                    duplicate_res->set_needs_gpu_compute(original->get_needs_gpu_compute());
                    duplicate_res->set_enable_shadowing(original->get_enable_shadowing());
                    duplicate_res->set_selection_mode(original->get_selection_mode());
                    duplicate_res->set_columns(original->get_columns().duplicate(true));

                    new_schemas.push_back(duplicate_res);
                }
            }

            for (int i = safe_target + 1; i < old_schemas.size(); ++i) {
                new_schemas.push_back(old_schemas[i]);
            }

            undo_redo->create_action("Duplicate Buffers (Bulk)");
            undo_redo->add_do_method(Callable(active_resource.ptr(), "set_buffer_schemas").bind(new_schemas));
            undo_redo->add_undo_method(Callable(active_resource.ptr(), "set_buffer_schemas").bind(old_schemas.duplicate()));
            undo_redo->commit_action();
        } else {
            active_resource->duplicate_buffers_bulk(clipboard_buffer_ids, target_index);
        }
    }
}

void MemoryProfiler::_clear_clipboard() {
    clipboard_buffer_ids.clear();
    is_cut_operation = false;
    memory_ribbon->clear_cut_buffers();
}

void MemoryProfiler::_on_ribbon_drag_initiated(int p_origin_buffer_id) {
    if (!selected_buffer_ids.has(p_origin_buffer_id)) {
        PackedInt32Array new_selection;
        new_selection.append(p_origin_buffer_id);
        _update_buffer_selection(new_selection);
    }
    _buffer_cut();
}

void MemoryProfiler::_on_ribbon_drop_paste_requested(int p_target_index) {
    last_clicked_buffer_index = p_target_index;
    _buffer_paste();
}

void MemoryProfiler::_request_delete_selection(bool p_bypass_confirm) {
    if (active_resource.is_null() || selected_buffer_ids.is_empty()) return;

    if (p_bypass_confirm) {
        _execute_delete();
    } else {
        delete_confirm_dialog->popup_centered();
    }
}

void MemoryProfiler::_on_delete_confirmed() {
    _execute_delete();
}

void MemoryProfiler::_execute_delete() {
    if (active_resource.is_null() || selected_buffer_ids.is_empty()) return;

    if (Engine::get_singleton()->is_editor_hint()) {
        if (undo_redo.is_valid()) {
            TypedArray<MemoryBufferResource> old_schemas = active_resource->get_buffer_schemas();
            TypedArray<MemoryBufferResource> new_schemas;
            for (int i = 0; i < old_schemas.size(); ++i) {
                if (!selected_buffer_ids.has(i)) {
                    new_schemas.push_back(old_schemas[i]);
                }
            }

            undo_redo->create_action("Delete Memory Buffers");
            undo_redo->add_do_method(Callable(active_resource.ptr(), "set_buffer_schemas").bind(new_schemas));
            undo_redo->add_undo_method(Callable(active_resource.ptr(), "set_buffer_schemas").bind(old_schemas.duplicate()));
            undo_redo->commit_action();
        } else {
            active_resource->remove_buffers_bulk(selected_buffer_ids);
        }
        _update_buffer_selection(PackedInt32Array());
    } else {
        emit_signal("runtime_delete_requested", selected_buffer_ids);
    }
}

void MemoryProfiler::_copy_resource_path() {
    if (active_resource.is_valid()) {
        String path = active_resource->get_path();
        if (!path.is_empty()) {
            DisplayServer::get_singleton()->clipboard_set(path);
        }
    }
}

void MemoryProfiler::_copy_resource_uid() {
    if (active_resource.is_valid()) {
        String path = active_resource->get_path();
        if (!path.is_empty()) {
            int64_t uid = ResourceLoader::get_singleton()->get_resource_uid(path);
            if (uid != ResourceUID::INVALID_ID) {
                String uid_text = ResourceUID::get_singleton()->id_to_text(uid);
                DisplayServer::get_singleton()->clipboard_set(uid_text);
            }
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

void MemoryProfiler::_on_ribbon_inspection_requested(int p_block_type, int p_index, bool p_shift_pressed, bool p_ctrl_pressed) {
    if (p_block_type == MemoryRibbon::BLOCK_BUFFER) {
        sidebar_tabs->set_current_tab(0);

        PackedInt32Array new_selection;

        if (p_shift_pressed && last_clicked_buffer_index != -1) {
            if (p_ctrl_pressed) {
                new_selection = selected_buffer_ids;
            }
            int start = (last_clicked_buffer_index < p_index) ? last_clicked_buffer_index : p_index;
            int end = (last_clicked_buffer_index > p_index) ? last_clicked_buffer_index : p_index;
            
            for (int i = start; i <= end; ++i) {
                if (!new_selection.has(i)) {
                    new_selection.append(i);
                }
            }
        } else if (p_ctrl_pressed) {
            new_selection = selected_buffer_ids;
            if (new_selection.has(p_index)) {
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
            new_selection.append(p_index);
            last_clicked_buffer_index = p_index;
        }

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
    for (int i = inspector_content->get_child_count() - 1; i >= 0; --i) {
        Node* child = inspector_content->get_child(i);
        if (child != inspector_title) {
            inspector_content->remove_child(child);
            child->queue_free();
        }
    }

    current_selection_view = memnew(MemoryBufferSelectionView);
    inspector_content->add_child(current_selection_view);
    inspector_title->set_text(String("Inspecting: Grant Part ") + String::num_int64(p_part_index));

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

    for (int i = 0; i < selected_buffer_ids.size(); ++i) {
        int idx = selected_buffer_ids[i];
        if (idx >= 0 && idx < memory_buffer_list->get_item_count()) {
            memory_buffer_list->select(idx, false);
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

void MemoryProfiler::_on_resource_changed() {
    _populate_ui();
}

void MemoryProfiler::open_resource(Ref<MemoryManagerResource> p_resource) {
    if (p_resource.is_null()) return;
    
    if (active_resource.is_valid() && active_resource->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        active_resource->disconnect("changed", Callable(this, "_on_resource_changed"));
    }

    active_resource = p_resource;
    active_resource->connect("changed", Callable(this, "_on_resource_changed"));

    active_resource->hook_buffer_signals();

    if (memory_ribbon) {
        memory_ribbon->sync_with_resource(active_resource);
    }

    _populate_ui();
}

void MemoryProfiler::close_resource() {
    if (active_resource.is_valid() && active_resource->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        active_resource->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
    active_resource.unref();
    
    memory_buffer_list->clear();
    memory_buffer_view->open_buffers(TypedArray<MemoryBufferResource>());
    
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