#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/v_separator.hpp>

#include "memory_buffer_resource.h"
#include "memory_manager_resource.h"
#include "memory_buffer_linkage_panel.h"
#include "aos_visualizer.h"
#include "soa_visualizer.h"
#include "tiled_soa_visualizer.h"
#include "sparse_set_visualizer.h"
#include "memory_page_grid.h"
#include "memory_buffer_column_panel.h"
#include "ring_visualizer.h"

#include "../controls/circular_gauge.h"
#include "../controls/heatmap_grid.h"

#include "../../core/memory/memory_buffer_pod.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class MemoryBufferView : public godot::VBoxContainer {
    GDCLASS(MemoryBufferView, godot::VBoxContainer)

public:
    using BufferLayoutType = ideam::core::BufferLayoutType;
    using BufferAlignmentMode = ideam::core::BufferAlignmentMode;
    using SelectionMode = ideam::godot_ext::MemoryBufferResource::SelectionMode;

private:
    godot::TypedArray<MemoryBufferResource> active_buffers;
    godot::Ref<MemoryManagerResource> manager_resource;

    // Status Display
    godot::Label* status_label = nullptr;

    // Header Toolbar
    godot::HBoxContainer* header_container = nullptr;
    godot::LineEdit* name_edit = nullptr;
    godot::OptionButton* layout_option = nullptr;
    godot::OptionButton* alignment_mode_option = nullptr;
    godot::SpinBox* max_elements_spin = nullptr;

    // Dynamic Layout Configuration Toolbar
    godot::HBoxContainer* layout_toolbar = nullptr;
    godot::OptionButton* page_preset_option = nullptr;
    godot::SpinBox* page_size_spin = nullptr;
    godot::OptionButton* access_strategy_option = nullptr;
    godot::Label* page_count_label = nullptr;

    godot::SpinBox* tile_elements_spin = nullptr;
    godot::Label* tile_count_label = nullptr;

    godot::SpinBox* sparse_universe_spin = nullptr;

    // Shadow Buffer Configuration Toolbar
    godot::HBoxContainer* shadow_toolbar = nullptr;
    godot::CheckBox* shadow_enable_check = nullptr;
    godot::OptionButton* selection_mode_option = nullptr;
    godot::Label* shadow_telemetry_label = nullptr;

    // Work Area Split
    godot::VSplitContainer* main_split = nullptr;
    godot::VBoxContainer* visualization_container = nullptr;
    
    // Lower Work Area Container (Column Panel on top, Linkage Panel collapsible at bottom)
    godot::VBoxContainer* lower_work_container = nullptr;
    MemoryBufferColumnPanel* column_panel = nullptr;
    MemoryBufferLinkagePanel* linkage_panel = nullptr;

    // Shadow Inspection Container & Dual-Tab System
    godot::TabContainer* shadow_tab_container = nullptr;
    HeatmapGrid* shadow_heatmap = nullptr;
    godot::PackedByteArray simulated_shadow_data;

    // Tab 2: Selection Details Tree & Pagination
    godot::Tree* selection_tree = nullptr;
    godot::LineEdit* details_filter_edit = nullptr;
    godot::CheckBox* show_selected_only_check = nullptr;
    godot::Label* details_summary_label = nullptr;
    godot::Button* btn_page_prev = nullptr;
    godot::Button* btn_page_next = nullptr;
    godot::Label* page_number_label = nullptr;

    int details_current_page = 0;
    const int DETAILS_PAGE_SIZE = 128;

    bool is_updating_ui = false;

    void _clear_view();
    void _build_view();
    void _update_layout_toolbar();
    void _update_shadow_toolbar();
    void _setup_shadow_inspection_ui(godot::Ref<MemoryBufferResource> p_buffer);
    void _rebuild_selection_tree();

protected:
    static void _bind_methods();

public:
    MemoryBufferView();
    ~MemoryBufferView();

    void open_buffers(const godot::TypedArray<MemoryBufferResource>& p_buffers);
    void set_manager_resource(const godot::Ref<MemoryManagerResource>& p_manager);
    godot::Ref<MemoryManagerResource> get_manager_resource() const;

    // Callbacks & Handlers
    void _on_buffer_changed();
    void _on_manager_changed();
    void _on_name_submitted(const godot::String& p_text);
    void _on_name_focus_exited();
    void _on_layout_type_selected(int p_index);
    void _on_alignment_mode_selected(int p_index);
    void _on_max_elements_changed(double p_value);
    
    // Layout Specific Toolbar Handlers
    void _on_page_preset_selected(int p_index);
    void _on_page_size_changed(double p_value);
    void _on_access_strategy_selected(int p_index);
    void _on_tile_elements_changed(double p_value);
    void _on_sparse_universe_changed(double p_value);

    // Shadow Buffer Handlers
    void _on_shadow_enable_toggled(bool p_enabled);
    void _on_selection_mode_selected(int p_index);

    // Heatmap Handlers
    void _on_heatmap_cell_clicked(int p_index, int p_button);
    void _on_heatmap_cell_hovered(int p_index);
    void _on_select_all_pressed();
    void _on_clear_selection_pressed();
    void _on_invert_selection_pressed();

    // Selection Details Tree Handlers
    void _on_details_tree_item_selected();
    void _on_filter_text_changed(const godot::String& p_text);
    void _on_show_selected_only_toggled(bool p_toggled);
    void _on_page_prev_pressed();
    void _on_page_next_pressed();
};

} // namespace ideam::godot_ext