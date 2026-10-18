#include "memory_buffer_tooltip.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryBufferTooltip::_bind_methods() {
    ClassDB::bind_method(D_METHOD("configure", "data"), &MemoryBufferTooltip::configure);
}

MemoryBufferTooltip::MemoryBufferTooltip() {
    set_mouse_filter(MOUSE_FILTER_IGNORE);

    Ref<StyleBoxFlat> bg_style;
    bg_style.instantiate();
    bg_style->set_bg_color(Color(0.12f, 0.13f, 0.16f, 0.96f));
    bg_style->set_border_width_all(1);
    bg_style->set_border_color(Color(0.28f, 0.32f, 0.40f, 1.0f));
    bg_style->set_corner_radius_all(4);
    bg_style->set_shadow_size(4);
    bg_style->set_shadow_color(Color(0.0f, 0.0f, 0.0f, 0.45f));
    add_theme_stylebox_override("panel", bg_style);

    root_margin = memnew(MarginContainer);
    root_margin->set_mouse_filter(MOUSE_FILTER_IGNORE);
    root_margin->add_theme_constant_override("margin_left", 10);
    root_margin->add_theme_constant_override("margin_top", 8);
    root_margin->add_theme_constant_override("margin_right", 10);
    root_margin->add_theme_constant_override("margin_bottom", 8);
    add_child(root_margin);

    content_box = memnew(VBoxContainer);
    content_box->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_theme_constant_override("separation", 6);
    root_margin->add_child(content_box);
}

void MemoryBufferTooltip::configure(const Dictionary& p_data) {
    for (int i = content_box->get_child_count() - 1; i >= 0; --i) {
        Node* child = content_box->get_child(i);
        content_box->remove_child(child);
        child->queue_free();
    }

    int block_type = p_data.get("block_type", 0);
    switch (block_type) {
        case 1: // BLOCK_MANAGED
            _configure_managed(p_data);
            break;
        case 2: // BLOCK_TRANSIENT
            _configure_transient(p_data);
            break;
        case 0: // BLOCK_BUFFER
        default:
            _configure_buffer(p_data);
            break;
    }
}

void MemoryBufferTooltip::_configure_buffer(const Dictionary& p_data) {
    int buffer_id = p_data.get("buffer_id", -1);
    String buffer_name = p_data.get("buffer_name", "Buffer");
    String layout_name = p_data.get("layout_type_name", "Raw");
    int64_t max_elements = p_data.get("max_elements", 0);
    int64_t alignment = p_data.get("alignment", 64);
    int64_t footprint_bytes = p_data.get("footprint_bytes", 0);
    bool is_shadowed = p_data.get("is_shadowed", false);
    String selection_mode = p_data.get("selection_mode_name", "Dense");
    int active_readers = p_data.get("active_readers", 0);
    int active_writers = p_data.get("active_writers", 0);
    Array columns = p_data.get("columns", Array());

    // 1. Header
    HBoxContainer* header_row = memnew(HBoxContainer);
    header_row->set_mouse_filter(MOUSE_FILTER_IGNORE);
    header_row->add_theme_constant_override("separation", 8);

    Label* name_label = memnew(Label);
    name_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
    name_label->set_text(buffer_name);
    header_row->add_child(name_label);

    header_row->add_child(_create_badge(String("[ID: ") + String::num_int64(buffer_id) + "]", Color(0.55f, 0.65f, 0.75f, 1.0f)));
    header_row->add_child(_create_badge(String("<") + layout_name + ">", Color(0.4f, 0.8f, 1.0f, 1.0f)));
    content_box->add_child(header_row);

    HSeparator* sep1 = memnew(HSeparator);
    sep1->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_child(sep1);

    // 2. Metrics
    GridContainer* metrics_grid = memnew(GridContainer);
    metrics_grid->set_mouse_filter(MOUSE_FILTER_IGNORE);
    metrics_grid->set_columns(2);
    metrics_grid->add_theme_constant_override("h_separation", 16);
    metrics_grid->add_theme_constant_override("v_separation", 2);

    metrics_grid->add_child(_create_meta_label("Elements:", String::num_int64(max_elements)));
    metrics_grid->add_child(_create_meta_label("Footprint:", _format_bytes(footprint_bytes), Color(0.45f, 0.95f, 0.55f, 1.0f)));
    metrics_grid->add_child(_create_meta_label("Alignment:", String::num_int64(alignment) + " B"));
    metrics_grid->add_child(_create_meta_label("Selection:", selection_mode));

    String shadow_text = is_shadowed ? "Enabled (Metadata SoA)" : "Disabled";
    Color shadow_color = is_shadowed ? Color(1.0f, 0.8f, 0.4f, 1.0f) : Color(0.6f, 0.6f, 0.6f, 1.0f);
    metrics_grid->add_child(_create_meta_label("Shadowing:", shadow_text, shadow_color));
    content_box->add_child(metrics_grid);

    // 3. Schema Columns
    if (!columns.is_empty()) {
        HSeparator* sep2 = memnew(HSeparator);
        sep2->set_mouse_filter(MOUSE_FILTER_IGNORE);
        content_box->add_child(sep2);

        Label* col_header = memnew(Label);
        col_header->set_mouse_filter(MOUSE_FILTER_IGNORE);
        col_header->set_text(String("Columns (") + String::num_int64(columns.size()) + "):");
        col_header->set_modulate(Color(0.7f, 0.75f, 0.85f, 1.0f));
        content_box->add_child(col_header);

        GridContainer* col_grid = memnew(GridContainer);
        col_grid->set_mouse_filter(MOUSE_FILTER_IGNORE);
        col_grid->set_columns(4);
        col_grid->add_theme_constant_override("h_separation", 12);
        col_grid->add_theme_constant_override("v_separation", 2);

        col_grid->add_child(_create_table_cell("ID", true));
        col_grid->add_child(_create_table_cell("Name", true));
        col_grid->add_child(_create_table_cell("Type", true));
        col_grid->add_child(_create_table_cell("Size", true));

        for (int i = 0; i < columns.size(); ++i) {
            Dictionary col = columns[i];
            int col_id = col.has("id") ? static_cast<int>(col["id"]) : i;
            String col_name = col.has("name") ? String(col["name"]) : String("col_") + String::num_int64(i);
            String col_type = col.has("data_type") ? _format_data_type(col["data_type"]) : "CUSTOM";
            int col_size = col.has("size") ? static_cast<int>(col["size"]) : 1;

            col_grid->add_child(_create_table_cell(String::num_int64(col_id), false, Color(0.6f, 0.6f, 0.6f, 1.0f)));
            col_grid->add_child(_create_table_cell(col_name, false, Color(0.9f, 0.9f, 0.9f, 1.0f)));
            col_grid->add_child(_create_table_cell(col_type, false, Color(0.4f, 0.75f, 1.0f, 1.0f)));
            col_grid->add_child(_create_table_cell(String::num_int64(col_size) + " B", false, Color(0.7f, 0.7f, 0.7f, 1.0f)));
        }
        content_box->add_child(col_grid);
    }

    // 4. Claims Telemetry
    HSeparator* sep3 = memnew(HSeparator);
    sep3->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_child(sep3);

    HBoxContainer* telemetry_row = memnew(HBoxContainer);
    telemetry_row->set_mouse_filter(MOUSE_FILTER_IGNORE);
    telemetry_row->add_theme_constant_override("separation", 6);

    Label* tele_tag = memnew(Label);
    tele_tag->set_mouse_filter(MOUSE_FILTER_IGNORE);
    tele_tag->set_text("Grant Status:");
    tele_tag->set_modulate(Color(0.6f, 0.65f, 0.75f, 1.0f));
    telemetry_row->add_child(tele_tag);

    Label* tele_status = memnew(Label);
    tele_status->set_mouse_filter(MOUSE_FILTER_IGNORE);

    if (active_readers == 0 && active_writers == 0) {
        tele_status->set_text("Idle (0 Claims)");
        tele_status->set_modulate(Color(0.55f, 0.55f, 0.55f, 1.0f));
    } else {
        tele_status->set_text(String::num_int64(active_writers) + " Writer(s), " + String::num_int64(active_readers) + " Reader(s)");
        if (active_writers > 1 || (active_writers >= 1 && active_readers > 0)) {
            tele_status->set_modulate(Color(1.0f, 0.4f, 0.35f, 1.0f));
        } else if (active_writers == 1) {
            tele_status->set_modulate(Color(1.0f, 0.75f, 0.3f, 1.0f));
        } else {
            tele_status->set_modulate(Color(0.35f, 0.85f, 1.0f, 1.0f));
        }
    }

    telemetry_row->add_child(tele_status);
    content_box->add_child(telemetry_row);
}

void MemoryBufferTooltip::_configure_managed(const Dictionary& p_data) {
    int index = p_data.get("block_index", 0);
    String consumer_name = p_data.get("consumer_name", "Managed Consumer");
    String purpose = p_data.get("purpose", "");
    String layout_name = p_data.get("layout_type_name", "Managed External");
    bool is_live = p_data.get("is_live", false);
    String lifecycle = p_data.get("lifecycle_state", "Offline");
    bool vram_dirty = p_data.get("is_vram_dirty", false);
    int64_t footprint = p_data.get("footprint_bytes", 0);
    int64_t used_bytes = p_data.get("used_bytes", 0);
    int64_t slack_bytes = p_data.get("slack_bytes", 0);
    double fragmentation = p_data.get("fragmentation_pct", 0.0);
    int64_t element_count = p_data.get("element_count", 0);
    int64_t max_elements = p_data.get("max_elements", 0);
    int64_t alignment = p_data.get("alignment", 64);
    int active_readers = p_data.get("active_readers", 0);
    int active_writers = p_data.get("active_writers", 0);
    Array columns = p_data.get("columns", Array());

    // 1. Header
    HBoxContainer* header_row = memnew(HBoxContainer);
    header_row->set_mouse_filter(MOUSE_FILTER_IGNORE);
    header_row->add_theme_constant_override("separation", 8);

    Label* name_label = memnew(Label);
    name_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
    name_label->set_text(consumer_name);
    header_row->add_child(name_label);

    header_row->add_child(_create_badge(String("[Profile #") + String::num_int64(index) + "]", Color(0.55f, 0.65f, 0.75f, 1.0f)));
    header_row->add_child(_create_badge(String("<") + layout_name + ">", Color(0.4f, 0.8f, 1.0f, 1.0f)));

    if (is_live) {
        header_row->add_child(_create_badge("[LIVE]", Color(0.45f, 0.95f, 0.55f, 1.0f)));
    } else {
        header_row->add_child(_create_badge("[AOT / OFFLINE]", Color(0.6f, 0.6f, 0.6f, 1.0f)));
    }

    if (vram_dirty) {
        header_row->add_child(_create_badge("[VRAM DIRTY]", Color(1.0f, 0.65f, 0.25f, 1.0f)));
    }

    content_box->add_child(header_row);

    HSeparator* sep1 = memnew(HSeparator);
    sep1->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_child(sep1);

    // 2. Telemetry Grid
    GridContainer* metrics_grid = memnew(GridContainer);
    metrics_grid->set_mouse_filter(MOUSE_FILTER_IGNORE);
    metrics_grid->set_columns(2);
    metrics_grid->add_theme_constant_override("h_separation", 16);
    metrics_grid->add_theme_constant_override("v_separation", 2);

    metrics_grid->add_child(_create_meta_label("Purpose:", purpose.is_empty() ? String("General Compute") : purpose));
    metrics_grid->add_child(_create_meta_label("Lifecycle:", lifecycle, is_live ? Color(0.45f, 0.95f, 0.55f, 1.0f) : Color(0.7f, 0.7f, 0.7f, 1.0f)));
    metrics_grid->add_child(_create_meta_label("Capacity:", _format_bytes(footprint), Color(0.45f, 0.95f, 0.55f, 1.0f)));
    metrics_grid->add_child(_create_meta_label("Alignment:", String::num_int64(alignment) + " B"));

    if (is_live) {
        metrics_grid->add_child(_create_meta_label("Used / Slack:", _format_bytes(used_bytes) + " / " + _format_bytes(slack_bytes)));
        metrics_grid->add_child(_create_meta_label("Elements:", String::num_int64(element_count) + " / " + String::num_int64(max_elements)));
        Color frag_color = (fragmentation > 20.0) ? Color(1.0f, 0.45f, 0.4f, 1.0f) : Color(0.85f, 0.85f, 0.85f, 1.0f);
        metrics_grid->add_child(_create_meta_label("Fragmentation:", String::num(fragmentation, 1) + "%", frag_color));
    } else {
        metrics_grid->add_child(_create_meta_label("Target Max:", String::num_int64(max_elements) + " items"));
    }

    content_box->add_child(metrics_grid);

    // 3. Schema Columns
    if (!columns.is_empty()) {
        HSeparator* sep2 = memnew(HSeparator);
        sep2->set_mouse_filter(MOUSE_FILTER_IGNORE);
        content_box->add_child(sep2);

        Label* col_header = memnew(Label);
        col_header->set_mouse_filter(MOUSE_FILTER_IGNORE);
        col_header->set_text(String("Bound Schema Columns (") + String::num_int64(columns.size()) + "):");
        col_header->set_modulate(Color(0.7f, 0.75f, 0.85f, 1.0f));
        content_box->add_child(col_header);

        GridContainer* col_grid = memnew(GridContainer);
        col_grid->set_mouse_filter(MOUSE_FILTER_IGNORE);
        col_grid->set_columns(4);
        col_grid->add_theme_constant_override("h_separation", 12);
        col_grid->add_theme_constant_override("v_separation", 2);

        col_grid->add_child(_create_table_cell("ID", true));
        col_grid->add_child(_create_table_cell("Name", true));
        col_grid->add_child(_create_table_cell("Type", true));
        col_grid->add_child(_create_table_cell("Size", true));

        for (int i = 0; i < columns.size(); ++i) {
            Dictionary col = columns[i];
            int col_id = col.has("id") ? static_cast<int>(col["id"]) : i;
            String col_name = col.has("name") ? String(col["name"]) : String("col_") + String::num_int64(i);
            String col_type = col.has("data_type") ? _format_data_type(col["data_type"]) : "CUSTOM";
            int col_size = col.has("size") ? static_cast<int>(col["size"]) : 1;

            col_grid->add_child(_create_table_cell(String::num_int64(col_id), false, Color(0.6f, 0.6f, 0.6f, 1.0f)));
            col_grid->add_child(_create_table_cell(col_name, false, Color(0.9f, 0.9f, 0.9f, 1.0f)));
            col_grid->add_child(_create_table_cell(col_type, false, Color(0.4f, 0.75f, 1.0f, 1.0f)));
            col_grid->add_child(_create_table_cell(String::num_int64(col_size) + " B", false, Color(0.7f, 0.7f, 0.7f, 1.0f)));
        }
        content_box->add_child(col_grid);
    }

    // 4. Access Telemetry
    HSeparator* sep3 = memnew(HSeparator);
    sep3->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_child(sep3);

    HBoxContainer* telemetry_row = memnew(HBoxContainer);
    telemetry_row->set_mouse_filter(MOUSE_FILTER_IGNORE);
    telemetry_row->add_theme_constant_override("separation", 6);

    Label* tele_tag = memnew(Label);
    tele_tag->set_mouse_filter(MOUSE_FILTER_IGNORE);
    tele_tag->set_text("Consumer Access:");
    tele_tag->set_modulate(Color(0.6f, 0.65f, 0.75f, 1.0f));
    telemetry_row->add_child(tele_tag);

    Label* tele_status = memnew(Label);
    tele_status->set_mouse_filter(MOUSE_FILTER_IGNORE);

    if (active_readers == 0 && active_writers == 0) {
        tele_status->set_text("Unbound / Idle (0 Threads)");
        tele_status->set_modulate(Color(0.55f, 0.55f, 0.55f, 1.0f));
    } else {
        tele_status->set_text(String::num_int64(active_writers) + " Active Writer(s), " + String::num_int64(active_readers) + " Reader(s)");
        if (active_writers > 1 || (active_writers >= 1 && active_readers > 0)) {
            tele_status->set_modulate(Color(1.0f, 0.4f, 0.35f, 1.0f));
        } else if (active_writers == 1) {
            tele_status->set_modulate(Color(1.0f, 0.75f, 0.3f, 1.0f));
        } else {
            tele_status->set_modulate(Color(0.35f, 0.85f, 1.0f, 1.0f));
        }
    }

    telemetry_row->add_child(tele_status);
    content_box->add_child(telemetry_row);
}

void MemoryBufferTooltip::_configure_transient(const Dictionary& p_data) {
    int64_t transient_mb = p_data.get("transient_mb", 16);
    int64_t footprint = p_data.get("footprint_bytes", transient_mb * 1024 * 1024);
    int64_t total_arena_bytes = p_data.get("total_arena_bytes", 0);
    int64_t alignment = p_data.get("alignment", 64);
    String strategy = p_data.get("scaling_strategy", "Fixed");

    // 1. Header
    HBoxContainer* header_row = memnew(HBoxContainer);
    header_row->set_mouse_filter(MOUSE_FILTER_IGNORE);
    header_row->add_theme_constant_override("separation", 8);

    Label* name_label = memnew(Label);
    name_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
    name_label->set_text("Transient Memory Arena");
    header_row->add_child(name_label);

    header_row->add_child(_create_badge("<Linear Scratchpad>", Color(0.4f, 0.8f, 1.0f, 1.0f)));
    header_row->add_child(_create_badge("[Volatile Pool]", Color(0.55f, 0.65f, 0.75f, 1.0f)));
    content_box->add_child(header_row);

    HSeparator* sep1 = memnew(HSeparator);
    sep1->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_child(sep1);

    // 2. Metrics
    GridContainer* metrics_grid = memnew(GridContainer);
    metrics_grid->set_mouse_filter(MOUSE_FILTER_IGNORE);
    metrics_grid->set_columns(2);
    metrics_grid->add_theme_constant_override("h_separation", 16);
    metrics_grid->add_theme_constant_override("v_separation", 2);

    metrics_grid->add_child(_create_meta_label("Pool Capacity:", _format_bytes(footprint) + " (" + String::num_int64(transient_mb) + " MB)", Color(0.45f, 0.95f, 0.55f, 1.0f)));
    metrics_grid->add_child(_create_meta_label("Scaling Strategy:", strategy, Color(0.4f, 0.85f, 1.0f, 1.0f)));

    double share = (total_arena_bytes > 0) 
        ? (static_cast<double>(footprint) / static_cast<double>(total_arena_bytes)) * 100.0 
        : 0.0;
    metrics_grid->add_child(_create_meta_label("Arena Share:", String::num(share, 1) + "% of total memory"));
    metrics_grid->add_child(_create_meta_label("Base Alignment:", String::num_int64(alignment) + " B"));
    metrics_grid->add_child(_create_meta_label("Allocation Policy:", "Monotonic Bump Pointer"));
    metrics_grid->add_child(_create_meta_label("Lifespan:", "Per-Frame / Dispatch Reset"));

    content_box->add_child(metrics_grid);

    // 3. Operational Note
    HSeparator* sep2 = memnew(HSeparator);
    sep2->set_mouse_filter(MOUSE_FILTER_IGNORE);
    content_box->add_child(sep2);

    Label* note = memnew(Label);
    note->set_mouse_filter(MOUSE_FILTER_IGNORE);
    note->set_text("Zero-fragmentation scratchpad memory. Reset each frame or dispatch cycle.\nDoes not incur schema serialization overhead.");
    note->set_modulate(Color(0.65f, 0.7f, 0.8f, 0.85f));
    content_box->add_child(note);
}

Label* MemoryBufferTooltip::_create_badge(const String& p_text, const Color& p_color) {
    Label* lbl = memnew(Label);
    lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
    lbl->set_text(p_text);
    lbl->set_modulate(p_color);
    return lbl;
}

Label* MemoryBufferTooltip::_create_meta_label(const String& p_key, const String& p_val, const Color& p_val_color) {
    Label* lbl = memnew(Label);
    lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
    lbl->set_text(p_key + String(" ") + p_val);
    lbl->set_modulate(p_val_color);
    return lbl;
}

Label* MemoryBufferTooltip::_create_table_cell(const String& p_text, bool p_is_header, const Color& p_custom_color) {
    Label* cell = memnew(Label);
    cell->set_mouse_filter(MOUSE_FILTER_IGNORE);
    cell->set_text(p_text);
    if (p_is_header) {
        cell->set_modulate(Color(0.55f, 0.65f, 0.75f, 1.0f));
    } else if (p_custom_color.a > 0.0f) {
        cell->set_modulate(p_custom_color);
    }
    return cell;
}

String MemoryBufferTooltip::_format_bytes(int64_t p_bytes) const {
    if (p_bytes < 1024) {
        return String::num_int64(p_bytes) + " B";
    }
    if (p_bytes < 1024 * 1024) {
        return String::num(static_cast<double>(p_bytes) / 1024.0, 2) + " KB";
    }
    return String::num(static_cast<double>(p_bytes) / (1024.0 * 1024.0), 2) + " MB";
}

String MemoryBufferTooltip::_format_data_type(const Variant& p_type_val) const {
    if (p_type_val.get_type() == Variant::STRING) {
        return p_type_val;
    }
    int64_t t = p_type_val;
    switch (t) {
        case 0: return "CUSTOM";
        case 1: return "INT8";
        case 2: return "UINT8";
        case 3: return "INT16";
        case 4: return "UINT16";
        case 5: return "INT32";
        case 6: return "UINT32";
        case 7: return "INT64";
        case 8: return "UINT64";
        case 9: return "FLOAT";
        case 10: return "DOUBLE";
        case 11: return "VECTOR2";
        case 12: return "VECTOR3";
        case 13: return "VECTOR4";
        case 14: return "TRANSFORM3D";
        default: return String("TYPE_") + String::num_int64(t);
    }
}

} // namespace ideam::godot_ext