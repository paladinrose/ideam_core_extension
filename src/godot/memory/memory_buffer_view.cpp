#include "memory_buffer_view.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryBufferView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_buffers", "buffers"), &MemoryBufferView::open_buffers);
    ClassDB::bind_method(D_METHOD("set_manager_resource", "manager"), &MemoryBufferView::set_manager_resource);
    ClassDB::bind_method(D_METHOD("get_manager_resource"), &MemoryBufferView::get_manager_resource);

    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &MemoryBufferView::_on_buffer_changed);
    ClassDB::bind_method(D_METHOD("_on_manager_changed"), &MemoryBufferView::_on_manager_changed);

    ClassDB::bind_method(D_METHOD("_on_name_submitted", "text"), &MemoryBufferView::_on_name_submitted);
    ClassDB::bind_method(D_METHOD("_on_name_focus_exited"), &MemoryBufferView::_on_name_focus_exited);
    ClassDB::bind_method(D_METHOD("_on_layout_type_selected", "index"), &MemoryBufferView::_on_layout_type_selected);
    ClassDB::bind_method(D_METHOD("_on_alignment_mode_selected", "index"), &MemoryBufferView::_on_alignment_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_max_elements_changed", "value"), &MemoryBufferView::_on_max_elements_changed);

    ClassDB::bind_method(D_METHOD("_on_page_preset_selected", "index"), &MemoryBufferView::_on_page_preset_selected);
    ClassDB::bind_method(D_METHOD("_on_page_size_changed", "value"), &MemoryBufferView::_on_page_size_changed);
    ClassDB::bind_method(D_METHOD("_on_access_strategy_selected", "index"), &MemoryBufferView::_on_access_strategy_selected);
    ClassDB::bind_method(D_METHOD("_on_tile_elements_changed", "value"), &MemoryBufferView::_on_tile_elements_changed);
    ClassDB::bind_method(D_METHOD("_on_sparse_universe_changed", "value"), &MemoryBufferView::_on_sparse_universe_changed);

    ClassDB::bind_method(D_METHOD("_on_shadow_enable_toggled", "enabled"), &MemoryBufferView::_on_shadow_enable_toggled);
    ClassDB::bind_method(D_METHOD("_on_selection_mode_selected", "index"), &MemoryBufferView::_on_selection_mode_selected);

    ClassDB::bind_method(D_METHOD("_on_heatmap_cell_clicked", "index", "button"), &MemoryBufferView::_on_heatmap_cell_clicked);
    ClassDB::bind_method(D_METHOD("_on_heatmap_cell_hovered", "index"), &MemoryBufferView::_on_heatmap_cell_hovered);
    ClassDB::bind_method(D_METHOD("_on_select_all_pressed"), &MemoryBufferView::_on_select_all_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_selection_pressed"), &MemoryBufferView::_on_clear_selection_pressed);
    ClassDB::bind_method(D_METHOD("_on_invert_selection_pressed"), &MemoryBufferView::_on_invert_selection_pressed);

    ClassDB::bind_method(D_METHOD("_on_details_tree_item_selected"), &MemoryBufferView::_on_details_tree_item_selected);
    ClassDB::bind_method(D_METHOD("_on_filter_text_changed", "text"), &MemoryBufferView::_on_filter_text_changed);
    ClassDB::bind_method(D_METHOD("_on_show_selected_only_toggled", "toggled"), &MemoryBufferView::_on_show_selected_only_toggled);
    ClassDB::bind_method(D_METHOD("_on_page_prev_pressed"), &MemoryBufferView::_on_page_prev_pressed);
    ClassDB::bind_method(D_METHOD("_on_page_next_pressed"), &MemoryBufferView::_on_page_next_pressed);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"), "set_manager_resource", "get_manager_resource");
}

MemoryBufferView::MemoryBufferView() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // 1. Fallback Status Label
    status_label = memnew(Label);
    status_label->set_theme_type_variation("HeaderLarge");
    status_label->set_text("No Buffer Selected");
    status_label->set_visible(true);
    add_child(status_label);

    // 2. Interactive Header Bar
    header_container = memnew(HBoxContainer);
    header_container->set_h_size_flags(SIZE_EXPAND_FILL);
    header_container->set_visible(false);
    add_child(header_container);

    name_edit = memnew(LineEdit);
    name_edit->set_h_size_flags(SIZE_SHRINK_BEGIN);
    name_edit->set_placeholder("Buffer Name");
    name_edit->connect("text_submitted", Callable(this, "_on_name_submitted"));
    name_edit->connect("focus_exited", Callable(this, "_on_name_focus_exited"));
    name_edit->set_custom_minimum_size(Vector2(20, 30));
    header_container->add_child(name_edit);

    layout_option = memnew(OptionButton);
    layout_option->add_item("Flat", static_cast<int>(BufferLayoutType::FLAT));
    layout_option->add_item("AoS", static_cast<int>(BufferLayoutType::AOS));
    layout_option->add_item("SoA", static_cast<int>(BufferLayoutType::SOA));
    layout_option->add_item("Sparse Set", static_cast<int>(BufferLayoutType::SPARSE_SET));
    layout_option->add_item("Tiled SoA", static_cast<int>(BufferLayoutType::TILED_SOA));
    layout_option->add_item("Ring", static_cast<int>(BufferLayoutType::RING));
    layout_option->add_item("Paged", static_cast<int>(BufferLayoutType::PAGED));
    layout_option->connect("item_selected", Callable(this, "_on_layout_type_selected"));
    layout_option->set_custom_minimum_size(Vector2(150, 30));
    header_container->add_child(layout_option);

    alignment_mode_option = memnew(OptionButton);
    alignment_mode_option->add_item("Tight", static_cast<int>(BufferAlignmentMode::TIGHT));
    alignment_mode_option->add_item("STD430", static_cast<int>(BufferAlignmentMode::STD430));
    alignment_mode_option->add_item("STD140", static_cast<int>(BufferAlignmentMode::STD140));
    alignment_mode_option->connect("item_selected", Callable(this, "_on_alignment_mode_selected"));
    header_container->add_child(alignment_mode_option);

    max_elements_spin = memnew(SpinBox);
    max_elements_spin->set_min(1);
    max_elements_spin->set_max(100000000);
    max_elements_spin->set_step(1);
    max_elements_spin->set_allow_greater(true);
    max_elements_spin->set_prefix("Max: ");
    max_elements_spin->set_suffix(" elems");
    max_elements_spin->set_tooltip_text("Maximum number of elements allocated in this buffer.");
    max_elements_spin->connect("value_changed", Callable(this, "_on_max_elements_changed"));
    max_elements_spin->set_custom_minimum_size(Vector2(150, 30));
    header_container->add_child(max_elements_spin);

    // 3. Dynamic Layout Configuration Toolbar
    layout_toolbar = memnew(HBoxContainer);
    layout_toolbar->set_h_size_flags(SIZE_EXPAND_FILL);
    layout_toolbar->set_visible(false);
    add_child(layout_toolbar);

    page_preset_option = memnew(OptionButton);
    page_preset_option->add_item("4 KB (Standard)", 4096);
    page_preset_option->add_item("16 KB (Medium)", 16384);
    page_preset_option->add_item("64 KB (Large)", 65536);
    page_preset_option->add_item("2 MB (Huge Page)", 2097152);
    page_preset_option->add_item("Custom", 0);
    page_preset_option->connect("item_selected", Callable(this, "_on_page_preset_selected"));
    layout_toolbar->add_child(page_preset_option);

    page_size_spin = memnew(SpinBox);
    page_size_spin->set_min(64);
    page_size_spin->set_max(16777216);
    page_size_spin->set_step(64);
    page_size_spin->set_prefix("Page: ");
    page_size_spin->set_suffix(" B");
    page_size_spin->connect("value_changed", Callable(this, "_on_page_size_changed"));
    layout_toolbar->add_child(page_size_spin);

    access_strategy_option = memnew(OptionButton);
    access_strategy_option->add_item("Linear Scan", 0);
    access_strategy_option->add_item("Random Scatter", 1);
    access_strategy_option->connect("item_selected", Callable(this, "_on_access_strategy_selected"));
    layout_toolbar->add_child(access_strategy_option);

    page_count_label = memnew(Label);
    page_count_label->set_text("Pages: 0");
    layout_toolbar->add_child(page_count_label);

    tile_elements_spin = memnew(SpinBox);
    tile_elements_spin->set_min(1);
    tile_elements_spin->set_max(4096);
    tile_elements_spin->set_prefix("Elems/Tile: ");
    tile_elements_spin->connect("value_changed", Callable(this, "_on_tile_elements_changed"));
    layout_toolbar->add_child(tile_elements_spin);

    tile_count_label = memnew(Label);
    tile_count_label->set_text("Tiles: 0");
    layout_toolbar->add_child(tile_count_label);

    sparse_universe_spin = memnew(SpinBox);
    sparse_universe_spin->set_min(1);
    sparse_universe_spin->set_max(10000000);
    sparse_universe_spin->set_prefix("Entity Universe: ");
    sparse_universe_spin->connect("value_changed", Callable(this, "_on_sparse_universe_changed"));
    layout_toolbar->add_child(sparse_universe_spin);

    // 4. Shadow Buffer Configuration Toolbar
    shadow_toolbar = memnew(HBoxContainer);
    shadow_toolbar->set_h_size_flags(SIZE_EXPAND_FILL);
    shadow_toolbar->set_visible(false);
    add_child(shadow_toolbar);

    shadow_enable_check = memnew(CheckBox);
    shadow_enable_check->set_text("Enable Shadowing");
    shadow_enable_check->set_tooltip_text("Allocate secondary shadow metadata buffer for selection masks, partitions, and LOD levels.");
    shadow_enable_check->connect("toggled", Callable(this, "_on_shadow_enable_toggled"));
    shadow_toolbar->add_child(shadow_enable_check);

    VSeparator* sep1 = memnew(VSeparator);
    shadow_toolbar->add_child(sep1);

    Label* sel_mode_lbl = memnew(Label);
    sel_mode_lbl->set_text("Selection Mode:");
    shadow_toolbar->add_child(sel_mode_lbl);

    selection_mode_option = memnew(OptionButton);
    selection_mode_option->add_item("Dense (64-bit Bitset)", static_cast<int>(SelectionMode::SELECTION_DENSE));
    selection_mode_option->add_item("Sparse (Compact IDs)", static_cast<int>(SelectionMode::SELECTION_SPARSE));
    selection_mode_option->connect("item_selected", Callable(this, "_on_selection_mode_selected"));
    shadow_toolbar->add_child(selection_mode_option);

    VSeparator* sep2 = memnew(VSeparator);
    shadow_toolbar->add_child(sep2);

    shadow_telemetry_label = memnew(Label);
    shadow_telemetry_label->set_h_size_flags(SIZE_EXPAND_FILL);
    shadow_telemetry_label->set_text("Shadow: Disabled (0 B)");
    shadow_toolbar->add_child(shadow_telemetry_label);

    // 5. Main Split Container (Visualizer above, Work Area below)
    main_split = memnew(VSplitContainer);
    main_split->set_h_size_flags(SIZE_EXPAND_FILL);
    main_split->set_v_size_flags(SIZE_EXPAND_FILL);
    main_split->set_split_offset(280);
    main_split->set_visible(false);
    add_child(main_split);

    visualization_container = memnew(VBoxContainer);
    visualization_container->set_h_size_flags(SIZE_EXPAND_FILL);
    visualization_container->set_v_size_flags(SIZE_EXPAND_FILL);
    main_split->add_child(visualization_container);

    // 6. Lower Work Area Container (Vertical nesting: Column Panel on top, Linkage Panel at bottom)
    lower_work_container = memnew(VBoxContainer);
    lower_work_container->set_h_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->set_v_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->add_theme_constant_override("separation", 10);
    main_split->add_child(lower_work_container);

    column_panel = memnew(MemoryBufferColumnPanel);
    column_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    column_panel->set_v_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->add_child(column_panel);

    linkage_panel = memnew(MemoryBufferLinkagePanel);
    linkage_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    lower_work_container->add_child(linkage_panel);
}

MemoryBufferView::~MemoryBufferView() {}

void MemoryBufferView::open_buffers(const TypedArray<MemoryBufferResource>& p_buffers) {
    for (int i = 0; i < active_buffers.size(); ++i) {
        Ref<MemoryBufferResource> buf = active_buffers[i];
        if (buf.is_valid() && buf->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
            buf->disconnect("changed", Callable(this, "_on_buffer_changed"));
        }
    }

    active_buffers = p_buffers;

    for (int i = 0; i < active_buffers.size(); ++i) {
        Ref<MemoryBufferResource> buf = active_buffers[i];
        if (buf.is_valid()) {
            buf->connect("changed", Callable(this, "_on_buffer_changed"));
        }
    }

    _build_view();
}

void MemoryBufferView::set_manager_resource(const Ref<MemoryManagerResource>& p_manager) {
    if (manager_resource == p_manager) return;
    if (manager_resource.is_valid() && manager_resource->is_connected("changed", Callable(this, "_on_manager_changed"))) {
        manager_resource->disconnect("changed", Callable(this, "_on_manager_changed"));
    }
    manager_resource = p_manager;
    if (manager_resource.is_valid()) {
        manager_resource->connect("changed", Callable(this, "_on_manager_changed"));
    }

    if (linkage_panel) {
        linkage_panel->set_manager_resource(manager_resource);
    }
}
Ref<MemoryManagerResource> MemoryBufferView::get_manager_resource() const { return manager_resource; }

void MemoryBufferView::_on_buffer_changed() {
    _build_view();
}

void MemoryBufferView::_on_manager_changed() {
    if (linkage_panel) {
        linkage_panel->refresh();
    }
}

void MemoryBufferView::_on_name_submitted(const godot::String& p_text) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) buf->set_buffer_name(p_text);
}

void MemoryBufferView::_on_name_focus_exited() {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid() && name_edit) buf->set_buffer_name(name_edit->get_text());
}

void MemoryBufferView::_on_layout_type_selected(int p_index) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_layout_type(layout_option->get_item_id(p_index));
    }
}

void MemoryBufferView::_on_alignment_mode_selected(int p_index) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_alignment_mode(alignment_mode_option->get_item_id(p_index));
    }
}

void MemoryBufferView::_on_max_elements_changed(double p_value) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_max_elements(static_cast<int>(p_value));
    }
}

void MemoryBufferView::_on_page_preset_selected(int p_index) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (!buf.is_valid()) return;

    int selected_bytes = page_preset_option->get_item_id(p_index);
    if (selected_bytes > 0) {
        buf->set_page_size_bytes(selected_bytes);
    }
}

void MemoryBufferView::_on_page_size_changed(double p_value) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_page_size_bytes(static_cast<int>(p_value));
    }
}

void MemoryBufferView::_on_access_strategy_selected(int p_index) {
    for (int i = 0; i < visualization_container->get_child_count(); ++i) {
        MemoryPageGrid* grid = Object::cast_to<MemoryPageGrid>(visualization_container->get_child(i));
        if (grid) {
            grid->set_access_strategy(p_index);
            break;
        }
    }
}

void MemoryBufferView::_on_tile_elements_changed(double p_value) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_elements_per_tile(static_cast<int>(p_value));
    }
}

void MemoryBufferView::_on_sparse_universe_changed(double p_value) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_sparse_universe_size(static_cast<int64_t>(p_value));
    }
}

void MemoryBufferView::_on_shadow_enable_toggled(bool p_enabled) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_enable_shadowing(p_enabled);
    }
}

void MemoryBufferView::_on_selection_mode_selected(int p_index) {
    if (is_updating_ui || active_buffers.size() != 1) return;
    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (buf.is_valid()) {
        buf->set_selection_mode(selection_mode_option->get_item_id(p_index));
    }
}

void MemoryBufferView::_update_layout_toolbar() {
    if (active_buffers.size() != 1) {
        layout_toolbar->set_visible(false);
        return;
    }

    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (!buf.is_valid()) return;

    BufferLayoutType layout = static_cast<BufferLayoutType>(buf->get_layout_type());

    bool is_paged = (layout == BufferLayoutType::PAGED);
    bool is_tiled = (layout == BufferLayoutType::TILED_SOA);
    bool is_sparse = (layout == BufferLayoutType::SPARSE_SET);

    page_preset_option->set_visible(is_paged);
    page_size_spin->set_visible(is_paged);
    access_strategy_option->set_visible(is_paged);
    page_count_label->set_visible(is_paged);

    tile_elements_spin->set_visible(is_tiled);
    tile_count_label->set_visible(is_tiled);

    sparse_universe_spin->set_visible(is_sparse);

    layout_toolbar->set_visible(is_paged || is_tiled || is_sparse);

    if (is_paged) {
        int cur_page_size = buf->get_page_size_bytes();
        page_size_spin->set_value(cur_page_size);

        int preset_idx = page_preset_option->get_item_index(cur_page_size);
        page_preset_option->select(preset_idx != -1 ? preset_idx : 4);

        page_count_label->set_text(String("Pages: ") + String::num_int64(buf->get_page_count()));
    } else if (is_tiled) {
        tile_elements_spin->set_value(buf->get_elements_per_tile());
        tile_count_label->set_text(String("Tiles: ") + String::num_int64(buf->get_tile_count()));
    } else if (is_sparse) {
        sparse_universe_spin->set_value(static_cast<double>(buf->get_sparse_universe_size()));
    }
}

void MemoryBufferView::_update_shadow_toolbar() {
    if (active_buffers.size() != 1) {
        shadow_toolbar->set_visible(false);
        return;
    }

    Ref<MemoryBufferResource> buf = active_buffers[0];
    if (!buf.is_valid()) return;

    shadow_toolbar->set_visible(true);

    bool shadowing = buf->get_enable_shadowing();
    shadow_enable_check->set_pressed_no_signal(shadowing);

    selection_mode_option->set_disabled(!shadowing);
    int mode_idx = selection_mode_option->get_item_index(buf->get_selection_mode());
    if (mode_idx != -1) selection_mode_option->select(mode_idx);

    if (shadowing) {
        int64_t max_elems = buf->get_max_elements();
        int shadow_bytes = buf->calculate_shadow_bytes();
        int projected_footprint = buf->calculate_projected_footprint_bytes();

        size_t selection_data_size = (buf->get_selection_mode() == SelectionMode::SELECTION_DENSE)
            ? (((max_elems + 63) / 64) * sizeof(uint64_t))
            : (max_elems * sizeof(int64_t));

        size_t meta_soa_size = (max_elems * sizeof(uint32_t)) + 
                               (max_elems * sizeof(int64_t)) + 
                               (max_elems * sizeof(uint8_t));

        shadow_telemetry_label->set_text(
            String("Shadow Footprint: ") + String::num_int64(shadow_bytes) + " B (" +
            "Sel: " + String::num_uint64(selection_data_size) + " B | " +
            "Meta SoA: " + String::num_uint64(meta_soa_size) + " B | " +
            "Pad: 256 B) -> Total: " + String::num_int64(projected_footprint) + " B"
        );
    } else {
        shadow_telemetry_label->set_text("Shadow: Disabled (0 B)");
    }
}

void MemoryBufferView::_clear_view() {
    for (int i = visualization_container->get_child_count() - 1; i >= 0; --i) {
        Node* child = visualization_container->get_child(i);
        visualization_container->remove_child(child);
        child->queue_free();
    }
    shadow_heatmap = nullptr;
    selection_tree = nullptr;
}

void MemoryBufferView::_build_view() {
    _clear_view();

    if (active_buffers.is_empty()) {
        status_label->set_text("No Buffer Selected");
        status_label->set_visible(true);
        header_container->set_visible(false);
        layout_toolbar->set_visible(false);
        shadow_toolbar->set_visible(false);
        main_split->set_visible(false);
        return;
    }

    if (active_buffers.size() > 1) {
        status_label->set_text(String("Multiple Buffers Selected (") + String::num_int64(active_buffers.size()) + ")");
        status_label->set_visible(true);
        header_container->set_visible(false);
        layout_toolbar->set_visible(false);
        shadow_toolbar->set_visible(false);
        main_split->set_visible(false);
        return;
    }

    Ref<MemoryBufferResource> active_buffer = active_buffers[0];
    if (!active_buffer.is_valid()) return;

    status_label->set_visible(false);
    header_container->set_visible(true);
    main_split->set_visible(true);

    is_updating_ui = true;

    name_edit->set_text(active_buffer->get_buffer_name());

    int layout_idx = layout_option->get_item_index(active_buffer->get_layout_type());
    if (layout_idx != -1) layout_option->select(layout_idx);

    int align_idx = alignment_mode_option->get_item_index(active_buffer->get_alignment_mode());
    if (align_idx != -1) alignment_mode_option->select(align_idx);

    max_elements_spin->set_value(active_buffer->get_max_elements());

    _update_layout_toolbar();
    _update_shadow_toolbar();

    column_panel->setup_from_resource(active_buffer);

    if (linkage_panel) {
        linkage_panel->setup(active_buffer, manager_resource);
    }

    is_updating_ui = false;

    // 1. Layout Visualizer Dispatch
    BufferLayoutType layout = static_cast<BufferLayoutType>(active_buffer->get_layout_type());
    switch (layout) {
        case BufferLayoutType::AOS: {
            AoSVisualizer* aos_vis = memnew(AoSVisualizer);
            aos_vis->setup_from_resource(active_buffer);
            visualization_container->add_child(aos_vis);
            break;
        }
        case BufferLayoutType::SOA: {
            SoAVisualizer* soa_vis = memnew(SoAVisualizer);
            soa_vis->setup_from_resource(active_buffer);
            visualization_container->add_child(soa_vis);
            break;
        }
        case BufferLayoutType::TILED_SOA: {
            TiledSoAVisualizer* tiled_vis = memnew(TiledSoAVisualizer);
            tiled_vis->setup_from_resource(active_buffer);
            visualization_container->add_child(tiled_vis);
            break;
        }
        case BufferLayoutType::SPARSE_SET: {
            SparseSetVisualizer* sparse_vis = memnew(SparseSetVisualizer);
            sparse_vis->setup_from_resource(active_buffer);
            visualization_container->add_child(sparse_vis);
            break;
        }
        case BufferLayoutType::PAGED: {
            MemoryPageGrid* page_vis = memnew(MemoryPageGrid);
            page_vis->setup_from_resource(active_buffer);
            page_vis->set_access_strategy(access_strategy_option->get_selected());
            visualization_container->add_child(page_vis);
            break;
        }
        case BufferLayoutType::RING: {
            RingVisualizer* ring_vis = memnew(RingVisualizer);
            ring_vis->setup_from_resource(active_buffer);
            visualization_container->add_child(ring_vis);
            break;
        }
        default: {
            Label* unsupported_lbl = memnew(Label);
            unsupported_lbl->set_text("Flat Buffer Linear Visualizer active.");
            visualization_container->add_child(unsupported_lbl);
            break;
        }
    }

    // 2. Selection Dual-Tab Inspector
    if (active_buffer->get_enable_shadowing()) {
        _setup_shadow_inspection_ui(active_buffer);
    }
}

void MemoryBufferView::_setup_shadow_inspection_ui(Ref<MemoryBufferResource> p_buffer) {
    if (!p_buffer.is_valid()) return;

    HSeparator* shadow_sep = memnew(HSeparator);
    visualization_container->add_child(shadow_sep);

    shadow_tab_container = memnew(TabContainer);
    shadow_tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    shadow_tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    shadow_tab_container->set_custom_minimum_size(Vector2(0, 240));
    visualization_container->add_child(shadow_tab_container);

    int max_elements = p_buffer->get_max_elements();
    if (simulated_shadow_data.size() != max_elements) {
        simulated_shadow_data.resize(max_elements);
        for (int i = 0; i < max_elements; ++i) {
            simulated_shadow_data.set(i, (i % 7 == 0 || i % 13 == 0) ? HeatmapGrid::STATE_SELECTED : HeatmapGrid::STATE_UNSELECTED);
        }
    }

    // TAB 1: SPATIAL HEATMAP
    VBoxContainer* heatmap_tab = memnew(VBoxContainer);
    heatmap_tab->set_name("Spatial Heatmap");
    heatmap_tab->set_h_size_flags(SIZE_EXPAND_FILL);
    heatmap_tab->set_v_size_flags(SIZE_EXPAND_FILL);
    shadow_tab_container->add_child(heatmap_tab);

    HBoxContainer* heatmap_bar = memnew(HBoxContainer);
    heatmap_bar->set_h_size_flags(SIZE_EXPAND_FILL);
    heatmap_tab->add_child(heatmap_bar);

    Label* matrix_lbl = memnew(Label);
    matrix_lbl->set_text("Bitset Matrix (64 cols/word):");
    heatmap_bar->add_child(matrix_lbl);

    Label* unsel_badge = memnew(Label);
    unsel_badge->set_text("[0] Unclaimed");
    unsel_badge->set_modulate(Color(0.6f, 0.6f, 0.65f));
    heatmap_bar->add_child(unsel_badge);

    Label* sel_badge = memnew(Label);
    sel_badge->set_text("[1] Active Claim");
    sel_badge->set_modulate(Color(0.12f, 0.75f, 0.90f));
    heatmap_bar->add_child(sel_badge);

    Label* hov_badge = memnew(Label);
    hov_badge->set_text("[2] Hovered");
    hov_badge->set_modulate(Color(0.98f, 0.78f, 0.15f));
    heatmap_bar->add_child(hov_badge);

    Button* btn_all = memnew(Button);
    btn_all->set_text("Select All");
    btn_all->connect("pressed", Callable(this, "_on_select_all_pressed"));
    heatmap_bar->add_child(btn_all);

    Button* btn_clear = memnew(Button);
    btn_clear->set_text("Clear");
    btn_clear->connect("pressed", Callable(this, "_on_clear_selection_pressed"));
    heatmap_bar->add_child(btn_clear);

    Button* btn_inv = memnew(Button);
    btn_inv->set_text("Invert");
    btn_inv->connect("pressed", Callable(this, "_on_invert_selection_pressed"));
    heatmap_bar->add_child(btn_inv);

    ScrollContainer* heatmap_scroll = memnew(ScrollContainer);
    heatmap_scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    heatmap_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    heatmap_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    heatmap_tab->add_child(heatmap_scroll);

    shadow_heatmap = memnew(HeatmapGrid);
    shadow_heatmap->set_h_size_flags(SIZE_EXPAND_FILL);
    shadow_heatmap->set_v_size_flags(SIZE_EXPAND_FILL);
    shadow_heatmap->set_columns(64);
    shadow_heatmap->set_square_cells(true);
    shadow_heatmap->set_data(simulated_shadow_data);
    shadow_heatmap->connect("cell_clicked", Callable(this, "_on_heatmap_cell_clicked"));
    shadow_heatmap->connect("cell_hovered", Callable(this, "_on_heatmap_cell_hovered"));
    heatmap_scroll->add_child(shadow_heatmap);

    // TAB 2: SELECTION DETAILS
    VBoxContainer* details_tab = memnew(VBoxContainer);
    details_tab->set_name("Selection Details");
    details_tab->set_h_size_flags(SIZE_EXPAND_FILL);
    details_tab->set_v_size_flags(SIZE_EXPAND_FILL);
    shadow_tab_container->add_child(details_tab);

    HBoxContainer* details_bar = memnew(HBoxContainer);
    details_bar->set_h_size_flags(SIZE_EXPAND_FILL);
    details_tab->add_child(details_bar);

    show_selected_only_check = memnew(CheckBox);
    show_selected_only_check->set_text("Show Active Claims Only");
    show_selected_only_check->set_pressed_no_signal(true);
    show_selected_only_check->connect("toggled", Callable(this, "_on_show_selected_only_toggled"));
    details_bar->add_child(show_selected_only_check);

    details_filter_edit = memnew(LineEdit);
    details_filter_edit->set_placeholder("Search index or entity ID...");
    details_filter_edit->set_h_size_flags(SIZE_EXPAND_FILL);
    details_filter_edit->connect("text_changed", Callable(this, "_on_filter_text_changed"));
    details_bar->add_child(details_filter_edit);

    details_summary_label = memnew(Label);
    details_summary_label->set_text("Claims: 0");
    details_bar->add_child(details_summary_label);

    btn_page_prev = memnew(Button);
    btn_page_prev->set_text("◀");
    btn_page_prev->connect("pressed", Callable(this, "_on_page_prev_pressed"));
    details_bar->add_child(btn_page_prev);

    page_number_label = memnew(Label);
    page_number_label->set_text("Page 1");
    details_bar->add_child(page_number_label);

    btn_page_next = memnew(Button);
    btn_page_next->set_text("▶");
    btn_page_next->connect("pressed", Callable(this, "_on_page_next_pressed"));
    details_bar->add_child(btn_page_next);

    selection_tree = memnew(Tree);
    selection_tree->set_h_size_flags(SIZE_EXPAND_FILL);
    selection_tree->set_v_size_flags(SIZE_EXPAND_FILL);
    selection_tree->set_columns(6);
    selection_tree->set_column_titles_visible(true);
    selection_tree->set_column_title(0, "Slot #");
    selection_tree->set_column_title(1, "Word : Bit");
    selection_tree->set_column_title(2, "Status");
    selection_tree->set_column_title(3, "Group Mask");
    selection_tree->set_column_title(4, "Partition ID");
    selection_tree->set_column_title(5, "LOD Level");
    selection_tree->connect("item_selected", Callable(this, "_on_details_tree_item_selected"));
    details_tab->add_child(selection_tree);

    _rebuild_selection_tree();
}

void MemoryBufferView::_rebuild_selection_tree() {
    if (!selection_tree) return;
    selection_tree->clear();

    TreeItem* root = selection_tree->create_item();
    int total_elements = simulated_shadow_data.size();
    if (total_elements == 0) return;

    bool active_only = show_selected_only_check ? show_selected_only_check->is_pressed() : true;
    String filter = details_filter_edit ? details_filter_edit->get_text().strip_edges() : "";

    Vector<int> matching_indices;
    for (int i = 0; i < total_elements; ++i) {
        uint8_t state = simulated_shadow_data[i];
        if (active_only && state != HeatmapGrid::STATE_SELECTED) continue;

        if (!filter.is_empty()) {
            String idx_str = String::num_int64(i);
            if (idx_str.find(filter) == -1) continue;
        }

        matching_indices.push_back(i);
    }

    if (details_summary_label) {
        details_summary_label->set_text(String("Visible: ") + String::num_int64(matching_indices.size()) + "/" + String::num_int64(total_elements));
    }

    int total_pages = MAX(1, (matching_indices.size() + DETAILS_PAGE_SIZE - 1) / DETAILS_PAGE_SIZE);
    details_current_page = CLAMP(details_current_page, 0, total_pages - 1);

    if (page_number_label) {
        page_number_label->set_text(String("Page ") + String::num_int64(details_current_page + 1) + "/" + String::num_int64(total_pages));
    }
    if (btn_page_prev) btn_page_prev->set_disabled(details_current_page <= 0);
    if (btn_page_next) btn_page_next->set_disabled(details_current_page >= total_pages - 1);

    int start_idx = details_current_page * DETAILS_PAGE_SIZE;
    int end_idx = MIN(start_idx + DETAILS_PAGE_SIZE, matching_indices.size());

    for (int k = start_idx; k < end_idx; ++k) {
        int idx = matching_indices[k];
        uint8_t state = simulated_shadow_data[idx];

        TreeItem* item = selection_tree->create_item(root);
        item->set_metadata(0, idx);

        item->set_text(0, String("#") + String::num_int64(idx));
        item->set_text(1, String::num_int64(idx / 64) + " : " + String::num_int64(idx % 64));

        String status_str = (state == HeatmapGrid::STATE_SELECTED) ? "ACTIVE WRITER" : "FREE";
        item->set_text(2, status_str);
        item->set_custom_color(2, (state == HeatmapGrid::STATE_SELECTED) ? Color(0.12f, 0.75f, 0.90f) : Color(0.6f, 0.6f, 0.65f));

        uint32_t group_mask = 1 << (idx % 8);
        int64_t partition = idx / 128;
        uint8_t lod = idx % 4;

        item->set_text(3, "0x" + String::num_uint64(group_mask, 16, true));
        item->set_text(4, "Partition_" + String::num_int64(partition));
        item->set_text(5, "LOD_" + String::num_int64(lod));
    }
}

void MemoryBufferView::_on_heatmap_cell_clicked(int p_index, int p_button) {
    if (p_index < 0 || p_index >= simulated_shadow_data.size() || !shadow_heatmap) return;

    uint8_t current_val = simulated_shadow_data[p_index];
    uint8_t new_val = (current_val == HeatmapGrid::STATE_SELECTED) ? HeatmapGrid::STATE_UNSELECTED : HeatmapGrid::STATE_SELECTED;

    simulated_shadow_data.set(p_index, new_val);
    shadow_heatmap->set_value(p_index, new_val);
    shadow_heatmap->set_selected_index(p_index);

    _rebuild_selection_tree();
}

void MemoryBufferView::_on_heatmap_cell_hovered(int p_index) {
    if (shadow_heatmap) {
        shadow_heatmap->set_hovered_index(p_index);
    }
}

void MemoryBufferView::_on_select_all_pressed() {
    for (int i = 0; i < simulated_shadow_data.size(); ++i) {
        simulated_shadow_data.set(i, HeatmapGrid::STATE_SELECTED);
    }
    if (shadow_heatmap) shadow_heatmap->set_data(simulated_shadow_data);
    _rebuild_selection_tree();
}

void MemoryBufferView::_on_clear_selection_pressed() {
    for (int i = 0; i < simulated_shadow_data.size(); ++i) {
        simulated_shadow_data.set(i, HeatmapGrid::STATE_UNSELECTED);
    }
    if (shadow_heatmap) shadow_heatmap->set_data(simulated_shadow_data);
    _rebuild_selection_tree();
}

void MemoryBufferView::_on_invert_selection_pressed() {
    for (int i = 0; i < simulated_shadow_data.size(); ++i) {
        uint8_t v = simulated_shadow_data[i];
        simulated_shadow_data.set(i, (v == HeatmapGrid::STATE_SELECTED) ? HeatmapGrid::STATE_UNSELECTED : HeatmapGrid::STATE_SELECTED);
    }
    if (shadow_heatmap) shadow_heatmap->set_data(simulated_shadow_data);
    _rebuild_selection_tree();
}

void MemoryBufferView::_on_details_tree_item_selected() {
    if (!selection_tree || !shadow_heatmap) return;
    TreeItem* item = selection_tree->get_selected();
    if (!item) return;

    int idx = static_cast<int>(item->get_metadata(0));
    shadow_heatmap->set_selected_index(idx);
}

void MemoryBufferView::_on_filter_text_changed(const godot::String& p_text) {
    details_current_page = 0;
    _rebuild_selection_tree();
}

void MemoryBufferView::_on_show_selected_only_toggled(bool p_toggled) {
    details_current_page = 0;
    _rebuild_selection_tree();
}

void MemoryBufferView::_on_page_prev_pressed() {
    if (details_current_page > 0) {
        details_current_page--;
        _rebuild_selection_tree();
    }
}

void MemoryBufferView::_on_page_next_pressed() {
    details_current_page++;
    _rebuild_selection_tree();
}

} // namespace ideam::godot_ext