#include "flat_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void FlatVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &FlatVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &FlatVisualizer::_on_buffer_changed);
}

FlatVisualizer::FlatVisualizer() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

FlatVisualizer::~FlatVisualizer() {
    _clear_connections();
}

void FlatVisualizer::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void FlatVisualizer::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    hovered_element = -1;
    queue_redraw();
    update_minimum_size();
}

void FlatVisualizer::_on_buffer_changed() {
    queue_redraw();
    update_minimum_size();
}

Color FlatVisualizer::_get_data_type_color(DataType p_type) const {
    if (has_theme_color("type_color", "FlatVisualizer")) {
        return get_theme_color("type_color", "FlatVisualizer");
    }

    switch (p_type) {
        case DataType::BOOL:        return Color(0.96f, 0.62f, 0.04f);
        case DataType::BYTE:        return Color(0.47f, 0.44f, 0.42f);
        case DataType::INT32:       return Color(0.06f, 0.72f, 0.51f);
        case DataType::INT64:       return Color(0.02f, 0.47f, 0.34f);
        case DataType::FLOAT32:     return Color(0.05f, 0.65f, 0.91f);
        case DataType::FLOAT64:     return Color(0.11f, 0.31f, 0.85f);
        case DataType::VECTOR2:
        case DataType::VECTOR3:
        case DataType::VECTOR4:     return Color(0.02f, 0.71f, 0.83f);
        case DataType::VECTOR2I:
        case DataType::VECTOR3I:
        case DataType::VECTOR4I:    return Color(0.08f, 0.72f, 0.65f);
        case DataType::VECTOR2D:
        case DataType::VECTOR3D:
        case DataType::VECTOR4D:    return Color(0.26f, 0.22f, 0.79f);
        case DataType::COLOR:       return Color(0.85f, 0.27f, 0.94f);
        case DataType::CUSTOM:      return Color(0.55f, 0.36f, 0.96f);
        default:                    return Color(0.35f, 0.35f, 0.38f);
    }
}

void FlatVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_element != -1) {
            hovered_element = -1;
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        if (!active_buffer.is_valid()) return;

        int max_elements = active_buffer->get_max_elements();
        if (max_elements <= 0) return;

        TypedArray<Dictionary> cols = active_buffer->get_columns();
        DataType primary_type = DataType::BYTE;
        int stride = active_buffer->get_element_stride();

        if (!cols.is_empty()) {
            Dictionary col0 = cols[0];
            int64_t dt_raw = col0.has("data_type") ? static_cast<int64_t>(col0["data_type"]) : static_cast<int64_t>(DataType::BYTE);
            primary_type = static_cast<DataType>(dt_raw);
        }
        if (stride <= 0) stride = 1;

        Vector2 size = get_size();
        float ruler_height = 20.0f;
        float track_margin_y = 4.0f;
        float bar_height = size.y - ruler_height - (track_margin_y * 2.0f);
        if (bar_height < 10.0f) bar_height = 10.0f;

        float track_y = ruler_height + track_margin_y;

        // Theme colors
        Color data_color = _get_data_type_color(primary_type);
        Color border_color = has_theme_color("border_color", "FlatVisualizer")
                           ? get_theme_color("border_color", "FlatVisualizer")
                           : Color(0.15f, 0.15f, 0.18f, 1.0f);
        Color boundary_color = has_theme_color("stride_separator_color", "FlatVisualizer")
                             ? get_theme_color("stride_separator_color", "FlatVisualizer")
                             : Color(0.0f, 0.0f, 0.0f, 0.45f);
        Color ruler_color = has_theme_color("ruler_color", "FlatVisualizer")
                          ? get_theme_color("ruler_color", "FlatVisualizer")
                          : Color(0.6f, 0.6f, 0.65f, 0.8f);
        Color highlight_color = Color(1.0f, 1.0f, 1.0f, 0.35f);

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 11;

        // 1. Draw Offset Ruler
        draw_line(Vector2(0, ruler_height - 2), Vector2(size.x, ruler_height - 2), ruler_color, 1.0f);

        int marker_divisions = 4;
        size_t total_bytes = static_cast<size_t>(stride) * static_cast<size_t>(max_elements);

        for (int i = 0; i <= marker_divisions; ++i) {
            float frac = static_cast<float>(i) / static_cast<float>(marker_divisions);
            float marker_x = frac * size.x;
            draw_line(Vector2(marker_x, ruler_height - 8), Vector2(marker_x, ruler_height - 2), ruler_color, 1.0f);

            if (font.is_valid()) {
                size_t byte_val = static_cast<size_t>(frac * static_cast<float>(total_bytes));
                String label = String::num_uint64(byte_val) + "B";
                Vector2 str_size = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);

                float text_x = marker_x + 2.0f;
                if (i == marker_divisions) {
                    text_x = marker_x - str_size.x - 2.0f;
                }
                draw_string(font, Vector2(text_x, ruler_height - 8.0f), label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, ruler_color);
            }
        }

        // 2. Main Contiguous Memory Ribbon
        Rect2 bar_rect(0.0f, track_y, size.x, bar_height);
        draw_rect(bar_rect, data_color);

        // 3. Stride Dividers
        float pixels_per_element = size.x / static_cast<float>(max_elements);

        if (pixels_per_element >= 3.0f) {
            for (int i = 1; i < max_elements; ++i) {
                float line_x = i * pixels_per_element;
                draw_line(Vector2(line_x, track_y), Vector2(line_x, track_y + bar_height), boundary_color, 1.0f);
            }
        } else {
            // Draw 64-byte cache-line ticks instead when elements are too dense
            size_t bytes_per_cache_line = 64;
            float pixels_per_byte = size.x / static_cast<float>(total_bytes > 0 ? total_bytes : 1);
            float pixels_per_cache_line = pixels_per_byte * static_cast<float>(bytes_per_cache_line);

            if (pixels_per_cache_line >= 4.0f) {
                for (float x = pixels_per_cache_line; x < size.x; x += pixels_per_cache_line) {
                    draw_line(Vector2(x, track_y), Vector2(x, track_y + bar_height), Color(0.0f, 0.0f, 0.0f, 0.25f), 1.0f);
                }
            }
        }

        // 4. Hover Highlight
        if (hovered_element >= 0 && hovered_element < max_elements) {
            float h_x = hovered_element * pixels_per_element;
            float h_w = Math::max(pixels_per_element, 2.0f);
            draw_rect(Rect2(h_x, track_y, h_w, bar_height), highlight_color);
        }

        // 5. Border
        draw_rect(bar_rect, border_color, false, 1.5f);
    }
}

void FlatVisualizer::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && active_buffer.is_valid()) {
        int max_elems = active_buffer->get_max_elements();
        Vector2 size = get_size();

        if (max_elems > 0 && size.x > 0.0f) {
            float rel_x = CLAMP(mm->get_position().x, 0.0f, size.x);
            int new_hovered = static_cast<int>((rel_x / size.x) * static_cast<float>(max_elems));
            new_hovered = CLAMP(new_hovered, 0, max_elems - 1);

            if (new_hovered != hovered_element) {
                hovered_element = new_hovered;
                queue_redraw();
            }
        }
    }
    Control::_gui_input(p_event);
}

String FlatVisualizer::_get_tooltip(const Vector2& p_at_position) const {
    if (!active_buffer.is_valid()) return String();

    int max_elems = active_buffer->get_max_elements();
    if (max_elems <= 0) return String();

    Vector2 size = get_size();
    if (size.x <= 0.0f) return String();

    float rel_x = CLAMP(p_at_position.x, 0.0f, size.x);
    int elem_idx = CLAMP(static_cast<int>((rel_x / size.x) * static_cast<float>(max_elems)), 0, max_elems - 1);

    int stride = active_buffer->get_element_stride();
    if (stride <= 0) stride = 1;

    size_t byte_offset = static_cast<size_t>(elem_idx) * static_cast<size_t>(stride);

    return String("Element: [") + String::num_int64(elem_idx) + " / " + String::num_int64(max_elems - 1) + "]\n" +
           "Byte Offset: 0x" + String::num_uint64(byte_offset, 16).to_upper() + " (" + String::num_uint64(byte_offset) + " B)\n" +
           "Stride: " + String::num_int64(stride) + " B";
}

Vector2 FlatVisualizer::_get_minimum_size() const {
    return Vector2(120.0f, 48.0f);
}

} // namespace ideam::godot_ext