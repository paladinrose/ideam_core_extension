#include "managed_buffer_view.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

using namespace godot;

namespace ideam::godot_ext {

void ManagedBufferView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_resource", "resource"), &ManagedBufferView::open_resource);
    ClassDB::bind_method(D_METHOD("get_resource"), &ManagedBufferView::get_resource);
    ClassDB::bind_method(D_METHOD("poll_telemetry"), &ManagedBufferView::poll_telemetry);
    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &ManagedBufferView::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_refresh_pressed"), &ManagedBufferView::_on_refresh_pressed);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_resource", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferResource"), "open_resource", "get_resource");
}

ManagedBufferView::ManagedBufferView() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    _setup_ui();
    set_process(true);
}

ManagedBufferView::~ManagedBufferView() {
    _clear_connections();
}

void ManagedBufferView::_clear_connections() {
    if (active_resource.is_valid() && active_resource->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        active_resource->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
}

void ManagedBufferView::_setup_ui() {
    // 1. Header Toolbar with Status Badges
    HBoxContainer* header_box = memnew(HBoxContainer);
    header_box->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(header_box);

    VBoxContainer* titles_box = memnew(VBoxContainer);
    titles_box->set_h_size_flags(SIZE_EXPAND_FILL);
    header_box->add_child(titles_box);

    header_title = memnew(Label);
    header_title->set_theme_type_variation("HeaderLarge");
    header_title->set_text("No Managed Buffer Selected");
    titles_box->add_child(header_title);

    purpose_label = memnew(Label);
    purpose_label->set_text("Select a ManagedBufferResource to inspect physical ground-truth metrics.");
    purpose_label->set_modulate(Color(0.70f, 0.70f, 0.75f));
    titles_box->add_child(purpose_label);

    // Status Badges
    HBoxContainer* badge_row = memnew(HBoxContainer);
    header_box->add_child(badge_row);

    badge_lifecycle = memnew(Label);
    badge_lifecycle->set_text("[IDLE]");
    badge_lifecycle->set_modulate(Color(0.20f, 0.85f, 0.55f));
    badge_row->add_child(badge_lifecycle);

    badge_mode = memnew(Label);
    badge_mode->set_text("[SCHEMA]");
    badge_mode->set_modulate(Color(0.12f, 0.75f, 0.90f));
    badge_row->add_child(badge_mode);

    badge_backend = memnew(Label);
    badge_backend->set_text("[OFFLINE]");
    badge_backend->set_modulate(Color(0.60f, 0.60f, 0.65f));
    badge_row->add_child(badge_backend);

    badge_locks = memnew(Label);
    badge_locks->set_text("[0W / 0R]");
    badge_locks->set_modulate(Color(0.96f, 0.62f, 0.15f));
    badge_row->add_child(badge_locks);

    // 2. Real-Time Telemetry Bar
    telemetry_strip = memnew(HBoxContainer);
    telemetry_strip->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(telemetry_strip);

    val_elements = memnew(Label);
    val_elements->set_text("Elements: 0 / 0 (0%)");
    telemetry_strip->add_child(val_elements);

    VSeparator* s1 = memnew(VSeparator);
    telemetry_strip->add_child(s1);

    val_footprint = memnew(Label);
    val_footprint->set_text("Capacity: 0 B | Used: 0 B");
    telemetry_strip->add_child(val_footprint);

    VSeparator* s2 = memnew(VSeparator);
    telemetry_strip->add_child(s2);

    val_slack = memnew(Label);
    val_slack->set_text("Slack: 0 B (0.0% frag)");
    telemetry_strip->add_child(val_slack);

    VSeparator* s3 = memnew(VSeparator);
    telemetry_strip->add_child(s3);

    val_layout_specs = memnew(Label);
    val_layout_specs->set_h_size_flags(SIZE_EXPAND_FILL);
    val_layout_specs->set_text("Layout: FLAT | Align: 64B");
    telemetry_strip->add_child(val_layout_specs);

    btn_refresh = memnew(Button);
    btn_refresh->set_text("↻ Poll");
    btn_refresh->set_tooltip_text("Query immediate physical POD state from MemoryManagerDOD.");
    btn_refresh->connect("pressed", Callable(this, "_on_refresh_pressed"));
    telemetry_strip->add_child(btn_refresh);

    // 3. Main Split Container (Visualizer on top, Work Area at bottom)
    main_split = memnew(VSplitContainer);
    main_split->set_h_size_flags(SIZE_EXPAND_FILL);
    main_split->set_v_size_flags(SIZE_EXPAND_FILL);
    main_split->set_split_offset(260);
    add_child(main_split);

    visualizer_container = memnew(VBoxContainer);
    visualizer_container->set_h_size_flags(SIZE_EXPAND_FILL);
    visualizer_container->set_v_size_flags(SIZE_EXPAND_FILL);
    main_split->add_child(visualizer_container);

    // 4. Lower Container (Columns Panel + Linkage Panel)
    lower_work_container = memnew(VBoxContainer);
    lower_work_container->set_h_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->set_v_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->add_theme_constant_override("separation", 8);
    main_split->add_child(lower_work_container);

    // Read-Only Columns Summary Card
    columns_card = memnew(PanelContainer);
    columns_card->set_h_size_flags(SIZE_EXPAND_FILL);
    Ref<StyleBoxFlat> col_card_style;
    col_card_style.instantiate();
    col_card_style->set_bg_color(Color(0.11f, 0.12f, 0.15f, 0.90f));
    col_card_style->set_border_width(SIDE_LEFT, 3);
    col_card_style->set_border_color(Color(0.45f, 0.40f, 0.85f));
    col_card_style->set_content_margin_all(6.0f);
    columns_card->add_theme_stylebox_override("panel", col_card_style);
    lower_work_container->add_child(columns_card);

    VBoxContainer* c_card_box = memnew(VBoxContainer);
    c_card_box->set_h_size_flags(SIZE_EXPAND_FILL);
    columns_card->add_child(c_card_box);

    HBoxContainer* c_header = memnew(HBoxContainer);
    c_header->set_h_size_flags(SIZE_EXPAND_FILL);
    c_card_box->add_child(c_header);

    Label* c_title = memnew(Label);
    c_title->set_text("Columns & Structure (Read-Only)");
    c_title->set_theme_type_variation("HeaderSmall");
    c_title->set_modulate(Color(0.65f, 0.60f, 0.95f));
    c_header->add_child(c_title);

    columns_rows_container = memnew(VBoxContainer);
    columns_rows_container->set_h_size_flags(SIZE_EXPAND_FILL);
    c_card_box->add_child(columns_rows_container);

    // Linkage Panel at Bottom
    linkage_panel = memnew(MemoryBufferLinkagePanel);
    linkage_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->add_child(linkage_panel);
}

void ManagedBufferView::open_resource(const Ref<ManagedBufferResource>& p_resource) {
    if (active_resource == p_resource) return;

    _clear_connections();
    active_resource = p_resource;

    if (active_resource.is_valid()) {
        active_resource->connect("changed", Callable(this, "_on_resource_changed"));
    }

    _rebuild_view();
}

Ref<ManagedBufferResource> ManagedBufferView::get_resource() const {
    return active_resource;
}

void ManagedBufferView::_on_resource_changed() {
    _rebuild_view();
}

void ManagedBufferView::_on_refresh_pressed() {
    poll_telemetry();
}

void ManagedBufferView::_notification(int p_what) {
    if (p_what == NOTIFICATION_PROCESS) {
        if (!active_resource.is_valid() || !active_resource->is_live()) return;

        poll_accumulator += get_process_delta_time();
        if (poll_accumulator >= POLL_INTERVAL) {
            poll_accumulator = 0.0;
            _update_telemetry_ui();
        }
    }
}

void ManagedBufferView::poll_telemetry() {
    _update_telemetry_ui();
}

void ManagedBufferView::_update_telemetry_ui() {
    if (!active_resource.is_valid()) return;

    // Update Badges
    int lifecycle = active_resource->get_lifecycle_state();
    badge_lifecycle->set_text(String("[") + active_resource->get_lifecycle_state_string() + "]");
    if (lifecycle == 0) badge_lifecycle->set_modulate(Color(0.20f, 0.85f, 0.55f));      // IDLE
    else if (lifecycle == 1) badge_lifecycle->set_modulate(Color(0.98f, 0.65f, 0.15f)); // LOCKED
    else badge_lifecycle->set_modulate(Color(0.95f, 0.25f, 0.25f));                     // MIGRATING

    bool is_live = active_resource->is_live();
    badge_backend->set_text(is_live ? "[LIVE BACKEND]" : "[OFFLINE / AOT]");
    badge_backend->set_modulate(is_live ? Color(0.20f, 0.85f, 0.55f) : Color(0.60f, 0.60f, 0.65f));

    badge_mode->set_text(active_resource->has_schema() ? "[SCHEMA BOUND]" : "[RAW POD]");
    badge_mode->set_modulate(active_resource->has_schema() ? Color(0.12f, 0.75f, 0.90f) : Color(0.72f, 0.35f, 0.95f));

    int writers = active_resource->get_active_writer_count();
    int readers = active_resource->get_active_reader_count();
    badge_locks->set_text(String("[") + String::num_int64(writers) + "W / " + String::num_int64(readers) + "R]");
    badge_locks->set_modulate(writers > 0 ? Color(0.98f, 0.65f, 0.15f) : Color(0.20f, 0.85f, 0.55f));

    // Update Telemetry Metrics
    int64_t count = active_resource->get_element_count();
    int64_t max_e = active_resource->get_max_elements();
    double elem_pct = (max_e > 0) ? (static_cast<double>(count) / static_cast<double>(max_e) * 100.0) : 0.0;
    val_elements->set_text(String("Elements: ") + String::num_int64(count) + " / " + String::num_int64(max_e) +
                           " (" + String::num_real(elem_pct, false) + "%)");

    int64_t cap_bytes = active_resource->get_capacity_bytes();
    int64_t used_bytes = active_resource->get_used_bytes();
    val_footprint->set_text(String("Capacity: ") + String::num_int64(cap_bytes) + " B | Used: " + String::num_int64(used_bytes) + " B");

    int64_t slack = active_resource->get_slack_bytes();
    double frag_pct = active_resource->get_fragmentation_percentage();
    val_slack->set_text(String("Slack: ") + String::num_int64(slack) + " B (" + String::num_real(frag_pct, false) + "% waste)");

    // Layout specs
    int layout_type = active_resource->get_effective_layout_type();
    String l_str = "FLAT";
    if (layout_type == 2) l_str = "AOS";
    else if (layout_type == 4) l_str = "SOA";
    else if (layout_type == 8) l_str = "SPARSE SET";
    else if (layout_type == 16) l_str = "TILED SOA";
    else if (layout_type == 32) l_str = "RING";
    else if (layout_type == 64) l_str = "PAGED";

    val_layout_specs->set_text(String("Layout: ") + l_str + " | Align: " + String::num_int64(active_resource->get_alignment()) + "B");
}

void ManagedBufferView::_rebuild_view() {
    for (int i = visualizer_container->get_child_count() - 1; i >= 0; --i) {
        Node* child = visualizer_container->get_child(i);
        visualizer_container->remove_child(child);
        child->queue_free();
    }

    if (!active_resource.is_valid()) {
        header_title->set_text("No Managed Buffer Selected");
        purpose_label->set_text("Select a ManagedBufferResource to inspect physical ground-truth metrics.");
        main_split->set_visible(false);
        return;
    }

    main_split->set_visible(true);

    // Update Titles
    header_title->set_text(active_resource->get_effective_buffer_name());
    String purpose = active_resource->get_purpose();
    purpose_label->set_text(purpose.is_empty() ? String("No purpose string defined.") : purpose);

    _update_telemetry_ui();

    Ref<MemoryBufferResource> effective_schema = active_resource->get_effective_schema();
    _route_visualizer(effective_schema);
    _rebuild_columns_card();

    if (linkage_panel) {
        linkage_panel->setup(effective_schema, active_resource->get_manager_resource());
    }
}

void ManagedBufferView::_route_visualizer(const Ref<MemoryBufferResource>& p_schema) {
    if (!p_schema.is_valid()) {
        Label* no_data = memnew(Label);
        no_data->set_text("Unable to resolve buffer schema or active POD slice.");
        visualizer_container->add_child(no_data);
        return;
    }

    int layout = active_resource->get_effective_layout_type();
    switch (layout) {
        case 2: { // AOS
            AoSVisualizer* aos_vis = memnew(AoSVisualizer);
            aos_vis->setup_from_resource(p_schema);
            visualizer_container->add_child(aos_vis);
            break;
        }
        case 4: { // SOA
            SoAVisualizer* soa_vis = memnew(SoAVisualizer);
            soa_vis->setup_from_resource(p_schema);
            visualizer_container->add_child(soa_vis);
            break;
        }
        case 8: { // SPARSE_SET
            SparseSetVisualizer* sparse_vis = memnew(SparseSetVisualizer);
            sparse_vis->setup_from_resource(p_schema);
            visualizer_container->add_child(sparse_vis);
            break;
        }
        case 16: { // TILED_SOA
            TiledSoAVisualizer* tiled_vis = memnew(TiledSoAVisualizer);
            tiled_vis->setup_from_resource(p_schema);
            visualizer_container->add_child(tiled_vis);
            break;
        }
        case 32: { // RING
            RingVisualizer* ring_vis = memnew(RingVisualizer);
            ring_vis->setup_from_resource(p_schema);
            visualizer_container->add_child(ring_vis);
            break;
        }
        case 64: { // PAGED
            MemoryPageGrid* page_vis = memnew(MemoryPageGrid);
            page_vis->setup_from_resource(p_schema);
            visualizer_container->add_child(page_vis);
            break;
        }
        default: {
            Label* flat_lbl = memnew(Label);
            flat_lbl->set_text("Flat Linear Buffer View Active.");
            visualizer_container->add_child(flat_lbl);
            break;
        }
    }
}

void ManagedBufferView::_rebuild_columns_card() {
    for (int i = columns_rows_container->get_child_count() - 1; i >= 0; --i) {
        Node* child = columns_rows_container->get_child(i);
        columns_rows_container->remove_child(child);
        child->queue_free();
    }

    Ref<MemoryBufferResource> schema = active_resource->get_effective_schema();
    if (!schema.is_valid()) return;

    TypedArray<Dictionary> cols = schema->get_columns();
    if (cols.is_empty()) {
        Label* empty_lbl = memnew(Label);
        empty_lbl->set_text("No columns configured (Unformatted byte blob).");
        empty_lbl->set_modulate(Color(0.55f, 0.55f, 0.60f));
        columns_rows_container->add_child(empty_lbl);
        return;
    }

    for (int i = 0; i < cols.size(); ++i) {
        Dictionary col = cols[i];
        String col_name = col.has("name") ? String(col["name"]) : ("col_" + String::num_int64(i));
        int size = col.has("size") ? static_cast<int>(col["size"]) : 4;
        int align = col.has("alignment") ? static_cast<int>(col["alignment"]) : 4;

        HBoxContainer* row = memnew(HBoxContainer);
        row->set_h_size_flags(SIZE_EXPAND_FILL);

        Label* idx_lbl = memnew(Label);
        idx_lbl->set_text(String("#") + String::num_int64(i));
        idx_lbl->set_modulate(Color(0.50f, 0.50f, 0.55f));
        row->add_child(idx_lbl);

        Label* name_lbl = memnew(Label);
        name_lbl->set_text(col_name);
        name_lbl->set_h_size_flags(SIZE_EXPAND_FILL);
        row->add_child(name_lbl);

        Label* specs_lbl = memnew(Label);
        specs_lbl->set_text(String("[Size: ") + String::num_int64(size) + " B | Align: " + String::num_int64(align) + " B]");
        specs_lbl->set_modulate(Color(0.70f, 0.70f, 0.75f));
        row->add_child(specs_lbl);

        columns_rows_container->add_child(row);
    }
}

} // namespace ideam::godot_ext