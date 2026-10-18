#include "aos_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void AoSVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &AoSVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &AoSVisualizer::_on_buffer_changed);
}

AoSVisualizer::AoSVisualizer() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

AoSVisualizer::~AoSVisualizer() {
    _clear_connections();
}

void AoSVisualizer::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void AoSVisualizer::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    hovered_block_index = -1;
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

void AoSVisualizer::_on_buffer_changed() {
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

Color AoSVisualizer::_get_type_color(DataType p_type) const {
    switch (p_type) {
        case DataType::BOOL:
            return has_theme_color("type_bool", "MemoryBufferTheme") 
                 ? get_theme_color("type_bool", "MemoryBufferTheme") 
                 : Color(0.96f, 0.62f, 0.04f); // Amber / Gold
        case DataType::BYTE:
            return has_theme_color("type_byte", "MemoryBufferTheme") 
                 ? get_theme_color("type_byte", "MemoryBufferTheme") 
                 : Color(0.47f, 0.44f, 0.42f); // Warm Gray / Bronze
        case DataType::INT32:
            return has_theme_color("type_int32", "MemoryBufferTheme") 
                 ? get_theme_color("type_int32", "MemoryBufferTheme") 
                 : Color(0.06f, 0.72f, 0.51f); // Emerald Green
        case DataType::INT64:
            return has_theme_color("type_int64", "MemoryBufferTheme") 
                 ? get_theme_color("type_int64", "MemoryBufferTheme") 
                 : Color(0.02f, 0.47f, 0.34f); // Deep Forest Green
        case DataType::FLOAT32:
            return has_theme_color("type_float32", "MemoryBufferTheme") 
                 ? get_theme_color("type_float32", "MemoryBufferTheme") 
                 : Color(0.05f, 0.65f, 0.91f); // Sky Blue
        case DataType::FLOAT64:
            return has_theme_color("type_float64", "MemoryBufferTheme") 
                 ? get_theme_color("type_float64", "MemoryBufferTheme") 
                 : Color(0.11f, 0.31f, 0.85f); // Deep Sapphire
        case DataType::VECTOR2:
        case DataType::VECTOR3:
        case DataType::VECTOR4:
            return has_theme_color("type_vec_float", "MemoryBufferTheme") 
                 ? get_theme_color("type_vec_float", "MemoryBufferTheme") 
                 : Color(0.02f, 0.71f, 0.83f); // Bright Cyan / Teal
        case DataType::VECTOR2I:
        case DataType::VECTOR3I:
        case DataType::VECTOR4I:
            return has_theme_color("type_vec_int", "MemoryBufferTheme") 
                 ? get_theme_color("type_vec_int", "MemoryBufferTheme") 
                 : Color(0.08f, 0.72f, 0.65f); // Seafoam / Mint
        case DataType::VECTOR2D:
        case DataType::VECTOR3D:
        case DataType::VECTOR4D:
            return has_theme_color("type_vec_double", "MemoryBufferTheme") 
                 ? get_theme_color("type_vec_double", "MemoryBufferTheme") 
                 : Color(0.26f, 0.22f, 0.79f); // Indigo / Royal Blue
        case DataType::COLOR:
            return has_theme_color("type_color", "MemoryBufferTheme") 
                 ? get_theme_color("type_color", "MemoryBufferTheme") 
                 : Color(0.85f, 0.27f, 0.94f); // Magenta / Orchid
        case DataType::CUSTOM:
            return has_theme_color("type_custom", "MemoryBufferTheme") 
                 ? get_theme_color("type_custom", "MemoryBufferTheme") 
                 : Color(0.55f, 0.36f, 0.96f); // Violet
        default:
            return Color(0.35f, 0.35f, 0.38f);
    }
}

String AoSVisualizer::_get_type_badge(DataType p_type) const {
    switch (p_type) {
        case DataType::BOOL:     return "[B1]";
        case DataType::BYTE:     return "[U1]";
        case DataType::INT32:    return "[I4]";
        case DataType::INT64:    return "[I8]";
        case DataType::FLOAT32:  return "[F4]";
        case DataType::FLOAT64:  return "[F8]";
        case DataType::VECTOR2:  return "[V2F]";
        case DataType::VECTOR3:  return "[V3F]";
        case DataType::VECTOR4:  return "[V4F]";
        case DataType::VECTOR2I: return "[V2I]";
        case DataType::VECTOR3I: return "[V3I]";
        case DataType::VECTOR4I: return "[V4I]";
        case DataType::VECTOR2D: return "[V2D]";
        case DataType::VECTOR3D: return "[V3D]";
        case DataType::VECTOR4D: return "[V4D]";
        case DataType::COLOR:    return "[C4]";
        case DataType::CUSTOM:   return "[C?]";
        default:                 return "[?]";
    }
}

String AoSVisualizer::_get_type_name(DataType p_type) const {
    switch (p_type) {
        case DataType::BOOL:     return "BOOL";
        case DataType::BYTE:     return "BYTE";
        case DataType::INT32:    return "INT32";
        case DataType::INT64:    return "INT64";
        case DataType::FLOAT32:  return "FLOAT32";
        case DataType::FLOAT64:  return "FLOAT64";
        case DataType::VECTOR2:  return "VECTOR2";
        case DataType::VECTOR3:  return "VECTOR3";
        case DataType::VECTOR4:  return "VECTOR4";
        case DataType::VECTOR2I: return "VECTOR2I";
        case DataType::VECTOR3I: return "VECTOR3I";
        case DataType::VECTOR4I: return "VECTOR4I";
        case DataType::VECTOR2D: return "VECTOR2D";
        case DataType::VECTOR3D: return "VECTOR3D";
        case DataType::VECTOR4D: return "VECTOR4D";
        case DataType::COLOR:    return "COLOR";
        case DataType::CUSTOM:   return "CUSTOM";
        default:                 return "UNKNOWN";
    }
}

String AoSVisualizer::_get_alignment_mode_name(BufferAlignmentMode p_mode) const {
    switch (p_mode) {
        case BufferAlignmentMode::TIGHT:  return "TIGHT";
        case BufferAlignmentMode::STD430: return "STD430";
        case BufferAlignmentMode::STD140: return "STD140";
        default:                          return "CUSTOM";
    }
}

void AoSVisualizer::_recalculate_layout() {
    cached_blocks.clear();
    total_stride_bytes = 0;
    max_struct_alignment = 1;
    total_padding_bytes = 0;

    if (!active_buffer.is_valid()) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    if (cols.is_empty()) return;

    size_t current_offset = 0;
    uint32_t max_align = 1;

    for (int i = 0; i < cols.size(); ++i) {
        Dictionary col = cols[i];

        int size = col.has("size") ? static_cast<int>(col["size"]) : 4;
        int align = col.has("alignment") ? static_cast<int>(col["alignment"]) : size;
        String name = col.has("name") ? String(col["name"]) : "field_" + String::num_int64(i);
        int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);
        DataType dt = static_cast<DataType>(raw_dt);

        align = Math::max(1, align);
        max_align = Math::max(max_align, static_cast<uint32_t>(align));

        size_t aligned_offset = ideam::core::MemoryUtilities::align_to(current_offset, align);
        int padding_before = static_cast<int>(aligned_offset - current_offset);

        if (padding_before > 0) {
            RenderBlock pad_block;
            pad_block.name = "Pad";
            pad_block.size_bytes = padding_before;
            pad_block.offset_bytes = static_cast<int>(current_offset);
            pad_block.alignment = align;
            pad_block.is_padding = true;
            pad_block.is_trailing_padding = false;
            cached_blocks.push_back(pad_block);

            total_padding_bytes += padding_before;
            current_offset = aligned_offset;
        }

        RenderBlock data_block;
        data_block.name = name;
        data_block.size_bytes = size;
        data_block.offset_bytes = static_cast<int>(current_offset);
        data_block.alignment = align;
        data_block.data_type = dt;
        data_block.is_padding = false;
        data_block.is_trailing_padding = false;
        cached_blocks.push_back(data_block);

        current_offset += static_cast<size_t>(size);
    }

    size_t final_stride = ideam::core::MemoryUtilities::align_to(current_offset, max_align);
    int trailing_padding = static_cast<int>(final_stride - current_offset);

    if (trailing_padding > 0) {
        RenderBlock trailing_block;
        trailing_block.name = "Trailing Pad";
        trailing_block.size_bytes = trailing_padding;
        trailing_block.offset_bytes = static_cast<int>(current_offset);
        trailing_block.alignment = static_cast<int>(max_align);
        trailing_block.is_padding = true;
        trailing_block.is_trailing_padding = true;
        cached_blocks.push_back(trailing_block);

        total_padding_bytes += trailing_padding;
        current_offset = final_stride;
    }

    total_stride_bytes = static_cast<int>(current_offset);
    max_struct_alignment = static_cast<int>(max_align);
}

void AoSVisualizer::_draw_crosshatch(const Rect2& p_rect, const Color& p_line_color, float p_spacing) {
    if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) return;

    float w = p_rect.size.x;
    float h = p_rect.size.y;

    // Forward diagonals (/)
    for (float d = -h; d < w; d += p_spacing) {
        float t_min = Math::max(0.0f, -d);
        float t_max = Math::min(h, w - d);
        if (t_min < t_max) {
            Vector2 p1(p_rect.position.x + d + t_min, p_rect.position.y + t_min);
            Vector2 p2(p_rect.position.x + d + t_max, p_rect.position.y + t_max);
            draw_line(p1, p2, p_line_color, 1.0f);
        }
    }

    // Backward diagonals (\)
    for (float d = -h; d < w; d += p_spacing) {
        float t_min = Math::max(0.0f, -d);
        float t_max = Math::min(h, w - d);
        if (t_min < t_max) {
            Vector2 p1(p_rect.position.x + d + t_min, p_rect.position.y + h - t_min);
            Vector2 p2(p_rect.position.x + d + t_max, p_rect.position.y + h - t_max);
            draw_line(p1, p2, p_line_color, 1.0f);
        }
    }
}

void AoSVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_block_index != -1) {
            hovered_block_index = -1;
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 12;

        if (!active_buffer.is_valid() || cached_blocks.is_empty() || total_stride_bytes <= 0) {
            Color placeholder_color = Color(0.5f, 0.5f, 0.55f, 0.8f);
            String msg = "No AoS structure defined. Add columns in the Column Panel.";
            if (font.is_valid()) {
                Vector2 text_sz = font->get_string_size(msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                draw_string(font, (size - text_sz) / 2.0f + Vector2(0, text_sz.y), msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, placeholder_color);
            }
            return;
        }

        // Layout Heights
        float header_h = 24.0f;
        float bar_h = 56.0f;
        float ruler_h = 18.0f;
        float footer_h = 24.0f;

        float bar_y = header_h + 4.0f;
        float ruler_y = bar_y + bar_h + 2.0f;
        float footer_y = ruler_y + ruler_h + 4.0f;

        BufferAlignmentMode align_mode = static_cast<BufferAlignmentMode>(active_buffer->get_alignment_mode());

        // Colors
        Color text_primary = has_theme_color("text_color", "AoSVisualizer") 
                           ? get_theme_color("text_color", "AoSVisualizer") 
                           : Color(0.95f, 0.95f, 0.95f, 1.0f);
        Color text_muted = Color(0.65f, 0.65f, 0.70f, 0.9f);
        Color border_color = has_theme_color("border_color", "AoSVisualizer") 
                           ? get_theme_color("border_color", "AoSVisualizer") 
                           : Color(0.12f, 0.12f, 0.14f, 1.0f);
        Color padding_bg = has_theme_color("padding_waste", "MemoryBufferTheme") 
                         ? get_theme_color("padding_waste", "MemoryBufferTheme") 
                         : Color(0.09f, 0.09f, 0.11f, 1.0f);
        Color crosshatch_color = Color(0.28f, 0.28f, 0.32f, 0.85f);
        Color hover_stroke_color = Color(0.95f, 0.85f, 0.35f, 1.0f);
        Color ruler_line_color = Color(0.35f, 0.35f, 0.40f, 0.8f);

        // --- 1. Draw Top Header Bar ---
        if (font.is_valid()) {
            String stride_info = "STRUCT STRIDE: " + String::num_int64(total_stride_bytes) + " B" +
                                 " | Max Alignment: " + String::num_int64(max_struct_alignment) + " B" +
                                 " | Padding: " + String::num_int64(total_padding_bytes) + " B";
            draw_string(font, Vector2(0.0f, header_h - 6.0f), stride_info, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_primary);

            String mode_info = "[" + _get_alignment_mode_name(align_mode) + "]";
            Vector2 mode_sz = font->get_string_size(mode_info, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size);
            draw_string(font, Vector2(size.x - mode_sz.x, header_h - 6.0f), mode_info, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, text_muted);
        }

        // --- 2. Draw Struct Stride Blocks ---
        float pixels_per_byte = size.x / static_cast<float>(total_stride_bytes);
        float cur_x = 0.0f;

        for (int i = 0; i < cached_blocks.size(); ++i) {
            RenderBlock& block = cached_blocks.write[i];
            float block_w = block.size_bytes * pixels_per_byte;
            block.rect = Rect2(cur_x, bar_y, block_w, bar_h);

            if (block.is_padding) {
                draw_rect(block.rect, padding_bg);
                _draw_crosshatch(block.rect, crosshatch_color, 8.0f);
            } else {
                Color field_col = _get_type_color(block.data_type);
                draw_rect(block.rect, field_col);
            }

            // Outer border
            draw_rect(block.rect, border_color, false, 1.5f);

            // Block Text Labels
            if (font.is_valid() && block_w > 12.0f) {
                String top_lbl;
                String btm_lbl;

                if (block.is_padding) {
                    top_lbl = block.is_trailing_padding ? "PAD [TR]" : "PAD";
                    btm_lbl = String::num_int64(block.size_bytes) + "B";
                } else {
                    top_lbl = block.name;
                    btm_lbl = _get_type_badge(block.data_type) + " " + String::num_int64(block.size_bytes) + "B";
                }

                Vector2 top_sz = font->get_string_size(top_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                Vector2 btm_sz = font->get_string_size(btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2);

                if (top_sz.x < (block_w - 6.0f)) {
                    Vector2 top_pos = block.rect.position + Vector2((block_w - top_sz.x) / 2.0f, (bar_h * 0.42f));
                    draw_string(font, top_pos + Vector2(1, 1), top_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, Color(0, 0, 0, 0.85f));
                    draw_string(font, top_pos, top_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, text_primary);

                    if (btm_sz.x < (block_w - 6.0f)) {
                        Vector2 btm_pos = block.rect.position + Vector2((block_w - btm_sz.x) / 2.0f, (bar_h * 0.80f));
                        draw_string(font, btm_pos + Vector2(1, 1), btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, Color(0, 0, 0, 0.85f));
                        draw_string(font, btm_pos, btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, text_muted);
                    }
                } else if (btm_sz.x < (block_w - 4.0f)) {
                    String mini_lbl = String::num_int64(block.size_bytes) + "B";
                    Vector2 mini_sz = font->get_string_size(mini_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2);
                    Vector2 mini_pos = block.rect.position + Vector2((block_w - mini_sz.x) / 2.0f, (bar_h * 0.58f));
                    draw_string(font, mini_pos + Vector2(1, 1), mini_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, Color(0, 0, 0, 0.85f));
                    draw_string(font, mini_pos, mini_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, text_primary);
                }
            }

            cur_x += block_w;
        }

        // --- 3. Draw Byte-Offset Ruler ---
        draw_line(Vector2(0.0f, ruler_y), Vector2(size.x, ruler_y), ruler_line_color, 1.0f);

        for (int i = 0; i < cached_blocks.size(); ++i) {
            const RenderBlock& b = cached_blocks[i];
            float mark_x = b.offset_bytes * pixels_per_byte;
            draw_line(Vector2(mark_x, ruler_y), Vector2(mark_x, ruler_y + 4.0f), ruler_line_color, 1.0f);

            if (font.is_valid() && (b.rect.size.x >= 24.0f || i == 0)) {
                String off_str = "+" + String::num_int64(b.offset_bytes);
                draw_string(font, Vector2(mark_x + 2.0f, ruler_y + 13.0f), off_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 3, text_muted);
            }
        }
        // Final stride tick
        draw_line(Vector2(size.x, ruler_y), Vector2(size.x, ruler_y + 4.0f), ruler_line_color, 1.0f);
        if (font.is_valid()) {
            String end_str = String::num_int64(total_stride_bytes) + "B";
            Vector2 end_sz = font->get_string_size(end_str, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size - 3);
            draw_string(font, Vector2(size.x - end_sz.x, ruler_y + 13.0f), end_str, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size - 3, text_muted);
        }

        // --- 4. Draw Hover Highlight ---
        if (hovered_block_index >= 0 && hovered_block_index < cached_blocks.size()) {
            const RenderBlock& h_block = cached_blocks[hovered_block_index];
            draw_rect(h_block.rect, hover_stroke_color, false, 2.5f);
        }

        // --- 5. Draw Footer Telemetry (Array Multiplier Footprint) ---
        if (font.is_valid()) {
            int64_t max_elems = active_buffer->get_max_elements();
            size_t total_footprint = static_cast<size_t>(total_stride_bytes) * static_cast<size_t>(max_elems);

            String footprint_str = "Total AoS Footprint: " + String::num_int64(total_stride_bytes) + " B × " +
                                   String::num_int64(max_elems) + " elements = " +
                                   String::num_uint64(total_footprint) + " Bytes (" +
                                   String::num_real(static_cast<double>(total_footprint) / 1024.0, false) + " KB)";

            draw_string(font, Vector2(0.0f, footer_y + 16.0f), footprint_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_primary);
        }
    }
}

void AoSVisualizer::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        Vector2 pos = mm->get_position();
        int new_hovered = -1;

        for (int i = 0; i < cached_blocks.size(); ++i) {
            if (cached_blocks[i].rect.has_point(pos)) {
                new_hovered = i;
                break;
            }
        }

        if (new_hovered != hovered_block_index) {
            hovered_block_index = new_hovered;
            queue_redraw();
        }
    }
    Control::_gui_input(p_event);
}

String AoSVisualizer::_get_tooltip(const Vector2& p_at_position) const {
    if (!active_buffer.is_valid()) return String();

    for (int i = 0; i < cached_blocks.size(); ++i) {
        const RenderBlock& b = cached_blocks[i];
        if (b.rect.has_point(p_at_position)) {
            if (b.is_trailing_padding) {
                return String("[Trailing Struct Padding]\n") +
                       "Offset: 0x" + String::num_uint64(b.offset_bytes, 16, true) + " (" + String::num_int64(b.offset_bytes) + " B)\n" +
                       "Size: " + String::num_int64(b.size_bytes) + " Bytes\n" +
                       "Rule: Pads struct stride to multiple of largest member alignment (" + String::num_int64(b.alignment) + " B)";
            } else if (b.is_padding) {
                return String("[Alignment Padding]\n") +
                       "Offset: 0x" + String::num_uint64(b.offset_bytes, 16, true) + " (" + String::num_int64(b.offset_bytes) + " B)\n" +
                       "Size: " + String::num_int64(b.size_bytes) + " Bytes\n" +
                       "Rule: Aligns next member to " + String::num_int64(b.alignment) + "-byte boundary";
            } else {
                int64_t max_elems = active_buffer->get_max_elements();
                size_t field_total = static_cast<size_t>(b.size_bytes) * static_cast<size_t>(max_elems);

                return String("Field: ") + b.name + "\n" +
                       "Type: " + _get_type_name(b.data_type) + " " + _get_type_badge(b.data_type) + "\n" +
                       "Offset: 0x" + String::num_uint64(b.offset_bytes, 16, true) + " (" + String::num_int64(b.offset_bytes) + " B)\n" +
                       "Size: " + String::num_int64(b.size_bytes) + " Bytes\n" +
                       "Alignment: " + String::num_int64(b.alignment) + " Bytes\n" +
                       "Total in Buffer: " + String::num_uint64(field_total) + " Bytes (" +
                       String::num_int64(b.size_bytes) + " B × " + String::num_int64(max_elems) + " elems)";
            }
        }
    }

    return Control::_get_tooltip(p_at_position);
}

Vector2 AoSVisualizer::_get_minimum_size() const {
    return Vector2(140.0f, 130.0f);
}

} // namespace ideam::godot_ext