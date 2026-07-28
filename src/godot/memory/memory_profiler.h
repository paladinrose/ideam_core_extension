#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/label.hpp>

#include "memory_manager_resource.h"
#include "memory_ribbon.h"
#include "memory_buffer_view.h"
#include "managed_buffer_view.h"
#include "../controls/theme_selector.h"

namespace ideam::godot_ext {

class MemoryProfiler : public godot::VBoxContainer {
    GDCLASS(MemoryProfiler, godot::VBoxContainer)

private:
    godot::Ref<MemoryManagerResource> active_resource;

    // Row 1: Control Bar
    godot::HBoxContainer* control_bar = nullptr;
    ThemeSelector* theme_selector_ui = nullptr;
    godot::Button* save_btn = nullptr;
    
    // Row 2: Ribbon
    MemoryRibbon* memory_ribbon = nullptr;
    
    // Row 3: Main Workspace
    godot::HBoxContainer* main_workspace = nullptr;
    
    // Column A: Sidebar
    godot::VBoxContainer* sidebar = nullptr;
    godot::ItemList* memory_buffer_list = nullptr;
    godot::Label* managed_buffer_list_title = nullptr;
    godot::ItemList* managed_buffer_list = nullptr;

    // Column B: Primary Visualization
    godot::PanelContainer* view_container = nullptr;
    MemoryBufferView* memory_buffer_view = nullptr;
    ManagedBufferView* managed_buffer_view = nullptr;        
    
    // Column C: Pseudo-Inspector
    godot::PanelContainer* inspector_panel = nullptr;
    godot::VBoxContainer* inspector_content = nullptr;
    godot::Label* inspector_title = nullptr;

    void _populate_ui();

protected:
    static void _bind_methods();
    static godot::Window* create_profiler_window();

public:
    MemoryProfiler();
    ~MemoryProfiler();

    // Signal Handlers
    void _on_save_pressed();
    void _on_buffer_item_selected(int p_index);
    void _on_managed_buffer_item_selected(int p_index);
    void _on_ribbon_inspection_requested(int p_block_type, int p_index);
    
    void _on_theme_applied(const godot::Ref<godot::Theme>& p_theme, int p_index);
    
    // Instance-level operations
    void open_resource(godot::Ref<MemoryManagerResource> p_resource);
    void close_resource();

    // Unified Static API routing (matches GraphComposer interface)
    static void profile_memory_manager(godot::Ref<MemoryManagerResource> p_resource, godot::Control* p_owner = nullptr);
    static void close_memory_profiler(godot::Ref<MemoryManagerResource> p_resource, godot::Control* p_owner = nullptr);
};

} // namespace ideam::godot_ext