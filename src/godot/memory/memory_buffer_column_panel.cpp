#include "memory_buffer_column_panel.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/theme_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

const MemoryBufferColumnPanel::LeafTypeInfo MemoryBufferColumnPanel::SUPPORTED_LEAF_TYPES[] = {
    { "BOOL",        DataType::BOOL },
    { "BYTE",        DataType::BYTE },
    { "INT32",       DataType::INT32 },
    { "INT64",       DataType::INT64 },
    { "FLOAT32",     DataType::FLOAT32 },
    { "FLOAT64",     DataType::FLOAT64 },
    { "VECTOR2",     DataType::VECTOR2 },
    { "VECTOR3",     DataType::VECTOR3 },
    { "VECTOR4",     DataType::VECTOR4 },
    { "VECTOR2I",    DataType::VECTOR2I },
    { "VECTOR3I",    DataType::VECTOR3I },
    { "VECTOR4I",    DataType::VECTOR4I },
    { "VECTOR2D",    DataType::VECTOR2D },
    { "VECTOR3D",    DataType::VECTOR3D },
    { "VECTOR4D",    DataType::VECTOR4D },
    { "COLOR",       DataType::COLOR },
    { "CUSTOM",      DataType::CUSTOM }
};

const size_t MemoryBufferColumnPanel::SUPPORTED_LEAF_TYPE_COUNT = 
    sizeof(MemoryBufferColumnPanel::SUPPORTED_LEAF_TYPES) / sizeof(MemoryBufferColumnPanel::LeafTypeInfo);

int MemoryBufferColumnPanel::_find_type_index(DataType p_type) {
    for (size_t i = 0; i < SUPPORTED_LEAF_TYPE_COUNT; ++i) {
        if (SUPPORTED_LEAF_TYPES[i].type == p_type) {
            return static_cast<int>(i);
        }
    }
    return 4; // Default to FLOAT32 index
}

void MemoryBufferColumnPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &MemoryBufferColumnPanel::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &MemoryBufferColumnPanel::_on_buffer_changed);
    ClassDB::bind_method(D_METHOD("_on_add_column_pressed"), &MemoryBufferColumnPanel::_on_add_column_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_columns_pressed"), &MemoryBufferColumnPanel::_on_clear_columns_pressed);
    ClassDB::bind_method(D_METHOD("_on_column_name_changed", "name", "index"), &MemoryBufferColumnPanel::_on_column_name_changed);
    ClassDB::bind_method(D_METHOD("_on_column_type_selected", "type_index", "col_index"), &MemoryBufferColumnPanel::_on_column_type_selected);
    ClassDB::bind_method(D_METHOD("_on_column_custom_size_changed", "size", "col_index"), &MemoryBufferColumnPanel::_on_column_custom_size_changed);
    ClassDB::bind_method(D_METHOD("_on_column_move_up", "index"), &MemoryBufferColumnPanel::_on_column_move_up);
    ClassDB::bind_method(D_METHOD("_on_column_move_down", "index"), &MemoryBufferColumnPanel::_on_column_move_down);
    ClassDB::bind_method(D_METHOD("_on_column_delete", "index"), &MemoryBufferColumnPanel::_on_column_delete);
}

MemoryBufferColumnPanel::MemoryBufferColumnPanel() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // 1. Header Bar
    header_bar = memnew(HBoxContainer);
    header_bar->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(header_bar);

    column_count_label = memnew(Label);
    column_count_label->set_text("Columns (0/16)");
    column_count_label->set_h_size_flags(SIZE_EXPAND_FILL);
    header_bar->add_child(column_count_label);

    add_column_btn = memnew(Button);
    add_column_btn->set_text("+ Add Column");
    add_column_btn->connect("pressed", Callable(this, "_on_add_column_pressed"));
    header_bar->add_child(add_column_btn);

    clear_columns_btn = memnew(Button);
    clear_columns_btn->set_text("Clear");
    clear_columns_btn->connect("pressed", Callable(this, "_on_clear_columns_pressed"));
    header_bar->add_child(clear_columns_btn);

    // 2. Scrollable Rows Area
   
    scroll_container = memnew(ScrollContainer);
    scroll_container->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll_container->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    
    add_child(scroll_container);

    rows_container = memnew(VBoxContainer);
    rows_container->set_h_size_flags(SIZE_EXPAND_FILL);
    rows_container->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll_container->add_child(rows_container);

    // 3. Footer Telemetry Bar
    footer_bar = memnew(HBoxContainer);
    footer_bar->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(footer_bar);

    footprint_label = memnew(Label);
    footprint_label->set_text("Stride: 0 B | Raw Data: 0 B | Padding Waste: 0 B (0.0%) | Shadow: 0 B | Master Block Footprint: 0 B");
    footprint_label->set_h_size_flags(SIZE_EXPAND_FILL);
    footer_bar->add_child(footprint_label);
}

MemoryBufferColumnPanel::~MemoryBufferColumnPanel() {
    _clear_connections();
}

void MemoryBufferColumnPanel::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void MemoryBufferColumnPanel::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
}

void MemoryBufferColumnPanel::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    _rebuild_ui();
}

void MemoryBufferColumnPanel::_on_buffer_changed() {
    if (is_updating_ui) return;
    _rebuild_ui();
}

void MemoryBufferColumnPanel::_rebuild_ui() {
    // Clear out old row controls
    for (int i = rows_container->get_child_count() - 1; i >= 0; --i) {
        Node* child = rows_container->get_child(i);
        rows_container->remove_child(child);
        child->queue_free();
    }

    if (!active_buffer.is_valid()) {
        column_count_label->set_text("Columns (0/16)");
        add_column_btn->set_disabled(true);
        clear_columns_btn->set_disabled(true);
        scroll_container->set_custom_minimum_size(Vector2(0, 30));
        _update_footer();
        return;
    }

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    int count = cols.size();
    int max_cols = static_cast<int>(ideam::core::MAX_BUFFER_COLUMNS);

    int max_rows_height = CLAMP(count * 30, 30, 120);
    scroll_container->set_custom_minimum_size(Vector2(0, max_rows_height));

    column_count_label->set_text(String("Columns (") + String::num_int64(count) + "/" + String::num_int64(max_cols) + ")");
    add_column_btn->set_disabled(count >= max_cols);
    clear_columns_btn->set_disabled(count == 0);

    for (int i = 0; i < count; ++i) {
        Dictionary col = cols[i];
        String col_name = col.has("name") ? String(col["name"]) : "unnamed";
        int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);
        DataType dt = static_cast<DataType>(raw_dt);
        int custom_size = col.has("custom_size") ? static_cast<int>(col["custom_size"]) : 0;
        int byte_size = col.has("size") ? static_cast<int>(col["size"]) : 4;
        int align_val = col.has("alignment") ? static_cast<int>(col["alignment"]) : 4;

        HBoxContainer* row = memnew(HBoxContainer);
        row->set_h_size_flags(SIZE_EXPAND_FILL);

        // Move Up Button
        Button* up_btn = memnew(Button);
        up_btn->set_text("▲");
        up_btn->set_disabled(i == 0);
        up_btn->connect("pressed", Callable(this, "_on_column_move_up").bind(i));
        row->add_child(up_btn);

        // Move Down Button
        Button* down_btn = memnew(Button);
        down_btn->set_text("▼");
        down_btn->set_disabled(i == count - 1);
        down_btn->connect("pressed", Callable(this, "_on_column_move_down").bind(i));
        row->add_child(down_btn);

        // Name Edit
        LineEdit* name_edit = memnew(LineEdit);
        name_edit->set_text(col_name);
        name_edit->set_placeholder("Column Name");
        name_edit->set_h_size_flags(SIZE_EXPAND_FILL);
        name_edit->connect("text_submitted", Callable(this, "_on_column_name_changed").bind(i));
        name_edit->connect("focus_exited", Callable(this, "_on_column_name_changed").bind(name_edit->get_text(), i));
        row->add_child(name_edit);

        // Type Option Dropdown
        OptionButton* type_option = memnew(OptionButton);
        for (size_t t = 0; t < SUPPORTED_LEAF_TYPE_COUNT; ++t) {
            type_option->add_item(SUPPORTED_LEAF_TYPES[t].name, static_cast<int>(t));
        }
        type_option->select(_find_type_index(dt));
        type_option->connect("item_selected", Callable(this, "_on_column_type_selected").bind(i));
        row->add_child(type_option);

        // Custom Size SpinBox (visible & editable strictly when CUSTOM is active)
        SpinBox* custom_spin = memnew(SpinBox);
        custom_spin->set_min(1);
        custom_spin->set_max(4096);
        custom_spin->set_value(custom_size > 0 ? custom_size : 4);
        custom_spin->set_visible(dt == DataType::CUSTOM);
        custom_spin->set_editable(dt == DataType::CUSTOM);
        custom_spin->set_prefix("Bytes: ");
        custom_spin->connect("value_changed", Callable(this, "_on_column_custom_size_changed").bind(i));
        row->add_child(custom_spin);

        // Computed Footprint & Alignment Badge
        Label* spec_badge = memnew(Label);
        spec_badge->set_text(String("[") + String::num_int64(byte_size) + "B | Aln:" + String::num_int64(align_val) + "]");
        spec_badge->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        row->add_child(spec_badge);

        // Delete Button
        Button* del_btn = memnew(Button);
        del_btn->set_text("✕");
        del_btn->connect("pressed", Callable(this, "_on_column_delete").bind(i));
        row->add_child(del_btn);

        rows_container->add_child(row);
    }

    _update_footer();
}

void MemoryBufferColumnPanel::_update_footer() {
    if (!active_buffer.is_valid()) {
        footprint_label->set_text("Stride: 0 B | Raw Data: 0 B | Padding Waste: 0 B (0.0%) | Shadow: 0 B | Master Block Footprint: 0 B");
        return;
    }

    int stride = active_buffer->get_element_stride();
    int raw_bytes = active_buffer->calculate_unpadded_data_bytes();
    int padding_bytes = active_buffer->calculate_padding_bytes();
    int shadow_bytes = active_buffer->calculate_shadow_bytes();
    int footprint = active_buffer->calculate_projected_footprint_bytes();

    double waste_pct = footprint > 0 ? (static_cast<double>(padding_bytes) / static_cast<double>(footprint) * 100.0) : 0.0;

    String info = "Stride: " + String::num_int64(stride) + " B | " +
                  "Raw Data: " + String::num_int64(raw_bytes) + " B | " +
                  "Padding Waste: " + String::num_int64(padding_bytes) + " B (" + String::num(waste_pct, 1) + "%) | " +
                  "Shadow: " + String::num_int64(shadow_bytes) + " B | " +
                  "Master Block Footprint: " + String::num_int64(footprint) + " B";
    footprint_label->set_text(info);
}

void MemoryBufferColumnPanel::_on_add_column_pressed() {
    if (!active_buffer.is_valid()) return;
    int current_count = active_buffer->get_columns().size();
    if (current_count >= static_cast<int>(ideam::core::MAX_BUFFER_COLUMNS)) return;

    String default_name = "col_" + String::num_int64(current_count);
    active_buffer->add_column(default_name, static_cast<int64_t>(DataType::FLOAT32), 0);
}

void MemoryBufferColumnPanel::_on_clear_columns_pressed() {
    if (!active_buffer.is_valid()) return;
    active_buffer->clear_columns();
}

void MemoryBufferColumnPanel::_on_column_name_changed(const godot::String& p_name, int p_index) {
    if (!active_buffer.is_valid()) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    if (p_index < 0 || p_index >= cols.size()) return;

    Dictionary col = cols[p_index];
    if (String(col["name"]) == p_name) return;

    is_updating_ui = true;
    col["name"] = p_name;
    cols[p_index] = col;
    active_buffer->set_columns(cols);
    is_updating_ui = false;
}

void MemoryBufferColumnPanel::_on_column_type_selected(int p_type_index, int p_col_index) {
    if (!active_buffer.is_valid() || p_type_index < 0 || p_type_index >= static_cast<int>(SUPPORTED_LEAF_TYPE_COUNT)) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    if (p_col_index < 0 || p_col_index >= cols.size()) return;

    DataType selected_dt = SUPPORTED_LEAF_TYPES[p_type_index].type;
    Dictionary col = cols[p_col_index];
    col["data_type"] = static_cast<int64_t>(selected_dt);

    if (selected_dt == DataType::CUSTOM && !col.has("custom_size")) {
        col["custom_size"] = 8;
    }

    cols[p_col_index] = col;
    active_buffer->set_columns(cols);
}

void MemoryBufferColumnPanel::_on_column_custom_size_changed(double p_size, int p_col_index) {
    if (!active_buffer.is_valid()) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    if (p_col_index < 0 || p_col_index >= cols.size()) return;

    Dictionary col = cols[p_col_index];
    col["custom_size"] = static_cast<int>(p_size);
    cols[p_col_index] = col;
    active_buffer->set_columns(cols);
}

void MemoryBufferColumnPanel::_on_column_move_up(int p_index) {
    if (!active_buffer.is_valid() || p_index <= 0) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    if (p_index >= cols.size()) return;

    Dictionary temp = cols[p_index - 1];
    cols[p_index - 1] = cols[p_index];
    cols[p_index] = temp;

    active_buffer->set_columns(cols);
}

void MemoryBufferColumnPanel::_on_column_move_down(int p_index) {
    if (!active_buffer.is_valid()) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    if (p_index < 0 || p_index >= cols.size() - 1) return;

    Dictionary temp = cols[p_index + 1];
    cols[p_index + 1] = cols[p_index];
    cols[p_index] = temp;

    active_buffer->set_columns(cols);
}

void MemoryBufferColumnPanel::_on_column_delete(int p_index) {
    if (!active_buffer.is_valid()) return;
    active_buffer->remove_column(p_index);
}

} // namespace ideam::godot_ext