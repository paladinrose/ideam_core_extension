#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/v_separator.hpp>

#include "managed_buffer_resource.h"
#include "memory_buffer_linkage_panel.h"
#include "aos_visualizer.h"
#include "soa_visualizer.h"
#include "tiled_soa_visualizer.h"
#include "sparse_set_visualizer.h"
#include "memory_page_grid.h"
#include "ring_visualizer.h"

namespace ideam::godot_ext {

class ManagedBufferView : public godot::VBoxContainer {
    GDCLASS(ManagedBufferView, godot::VBoxContainer)

private:
    godot::Ref<ManagedBufferResource> active_resource;

    // Header & Identification
    godot::Label* header_title = nullptr;
    godot::Label* purpose_label = nullptr;
    godot::Label* badge_lifecycle = nullptr;
    godot::Label* badge_mode = nullptr;
    godot::Label* badge_backend = nullptr;
    godot::Label* badge_locks = nullptr;

    // Real-Time Telemetry Bar
    godot::HBoxContainer* telemetry_strip = nullptr;
    godot::Label* val_elements = nullptr;
    godot::Label* val_footprint = nullptr;
    godot::Label* val_slack = nullptr;
    godot::Label* val_layout_specs = nullptr;
    godot::Button* btn_refresh = nullptr;

    // Work Area Split
    godot::VSplitContainer* main_split = nullptr;
    godot::VBoxContainer* visualizer_container = nullptr;

    // Lower Container (Read-Only Columns Table + Linkage Panel)
    godot::VBoxContainer* lower_work_container = nullptr;
    godot::PanelContainer* columns_card = nullptr;
    godot::VBoxContainer* columns_rows_container = nullptr;
    MemoryBufferLinkagePanel* linkage_panel = nullptr;

    // Polling Control
    double poll_accumulator = 0.0;
    const double POLL_INTERVAL = 0.1; // 10 Hz refresh

    void _clear_connections();
    void _setup_ui();
    void _rebuild_view();
    void _update_telemetry_ui();
    void _rebuild_columns_card();
    void _route_visualizer(const godot::Ref<MemoryBufferResource>& p_schema);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    ManagedBufferView();
    ~ManagedBufferView();

    void open_resource(const godot::Ref<ManagedBufferResource>& p_resource);
    godot::Ref<ManagedBufferResource> get_resource() const;

    void poll_telemetry();
    void _on_resource_changed();
    void _on_refresh_pressed();
};

} // namespace ideam::godot_ext