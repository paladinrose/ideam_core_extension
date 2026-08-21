#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

#include "memory_manager_resource.h"
#include "memory_ribbon.h"
#include "memory_buffer_view.h"
#include "managed_buffer_view.h"
#include "memory_grant_view.h"
#include "memory_buffer_selection_view.h"

#include "../controls/theme_selector.h"
#include "../utilities/ideam_undo_redo.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

namespace ideam::godot_ext {

class MemoryProfiler : public godot::VBoxContainer {
    GDCLASS(MemoryProfiler, godot::VBoxContainer)

private:
    godot::Ref<MemoryManagerResource> active_resource;
    godot::Ref<IdeamUndoRedo> undo_redo;

    // Row 1: Control Bar
    godot::HBoxContainer* control_bar = nullptr;
    ThemeSelector* theme_selector_ui = nullptr;
    godot::Button* save_btn = nullptr;
    
    // Row 2: Ribbon
    MemoryRibbon* memory_ribbon = nullptr;
    
    // Row 3: Main Workspace
    godot::HBoxContainer* main_workspace = nullptr;
    
    // Column A: Sidebar Tabs
    godot::TabContainer* sidebar_tabs = nullptr;
    
    godot::ItemList* memory_buffer_list = nullptr;
    godot::PackedInt32Array selected_buffer_ids;
    int last_clicked_buffer_index = -1;
    godot::PackedInt32Array clipboard_buffer_ids;
    bool is_cut_operation = false;

    godot::ItemList* managed_buffer_list = nullptr;
    godot::ItemList* memory_grant_list = nullptr;

    int hovered_grant_index = -1;

    // Column B: Primary Visualization
    godot::PanelContainer* view_container = nullptr;
    MemoryBufferView* memory_buffer_view = nullptr;
    ManagedBufferView* managed_buffer_view = nullptr;        
    MemoryGrantView* memory_grant_view = nullptr;
    
    // Column C: Pseudo-Inspector
    godot::PanelContainer* inspector_panel = nullptr;
    godot::VBoxContainer* inspector_content = nullptr;
    godot::Label* inspector_title = nullptr;
    MemoryBufferSelectionView* current_selection_view = nullptr;

    // Centralized update hub
    void _update_buffer_selection(const godot::PackedInt32Array& p_selection);
    void _populate_ui();

protected:
    static void _bind_methods();
    static godot::Window* create_profiler_window();

public:
    MemoryProfiler();
    ~MemoryProfiler();

#ifdef TOOLS_ENABLED
    void set_editor_undo_redo(godot::EditorUndoRedoManager* p_manager);
#endif
    godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo; }

    // Signal Handlers
    void _on_save_pressed();
    
    void _on_buffer_item_selected(int p_index);
    void _on_buffer_list_gui_input(const godot::Ref<godot::InputEvent>& p_event);
    void _select_all_buffers();
    void _invert_buffer_selection();
    void _buffer_cut();
    void _buffer_copy();
    void _buffer_paste();
    void _clear_clipboard();

    void _on_managed_buffer_item_selected(int p_index);
    void _on_grant_item_selected(int p_index);
    
    void _on_grant_list_gui_input(const godot::Ref<godot::InputEvent>& p_event);
    void _on_grant_list_mouse_exited();

    void _on_ribbon_inspection_requested(int p_block_type, int p_index, bool p_shift_pressed = false, bool p_ctrl_pressed = false);
    
    void _on_theme_applied(const godot::Ref<godot::Theme>& p_theme, int p_index);
    
    void _on_part_selection_inspection_requested(int p_part_index);
    
    // Instance-level operations
    void open_resource(godot::Ref<MemoryManagerResource> p_resource);
    void _open_resource(godot::Ref<MemoryManagerResource> p_resource);
    void close_resource();
    void _close_resource();

    // Unified Static API routing 
#ifdef TOOLS_ENABLED
    static void profile_memory_manager(godot::Ref<MemoryManagerResource> p_resource, godot::Control* p_owner = nullptr, godot::EditorUndoRedoManager* p_undo_redo = nullptr);
#else
    static void profile_memory_manager(godot::Ref<MemoryManagerResource> p_resource, godot::Control* p_owner = nullptr);
#endif

    static void close_memory_profiler(godot::Ref<MemoryManagerResource> p_resource, godot::Control* p_owner = nullptr);
};

} // namespace ideam::godot_ext