#include "soa_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void SoAVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &SoAVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &SoAVisualizer::_on_buffer_changed);

    ClassDB::bind_method(D_METHOD("set_needs_compaction", "needed"), &SoAVisualizer::set_needs_compaction);
    ClassDB::bind_method(D_METHOD("get_needs_compaction"), &SoAVisualizer::get_needs_compaction);

    ClassDB::bind_method(D_METHOD("set_active_element_count", "count"), &SoAVisualizer::set_active_element_count);
    ClassDB::bind_method(D_METHOD("get_active_element_count"), &SoAVisualizer::get_active_element_count);

    ClassDB::bind_method(D_METHOD("set_dead_element_count", "count"), &SoAVisualizer::set_dead_element_count);
    ClassDB::bind_method(D_METHOD("get_dead_element_count"), &SoAVisualizer::get_dead_element_count);

    ClassDB::bind_method(D_METHOD("set_tombstone_indices", "indices"), &SoAVisualizer::set_tombstone_indices);
    ClassDB::bind_method(D_METHOD("get_tombstone_indices"), &SoAVisualizer::get_tombstone_indices);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "needs_compaction"), "set_needs_compaction", "get_needs_compaction");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_element_count"), "set_active_element_count", "get_active_element_count");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dead_element_count"), "set_dead_element_count", "get_dead_element_count");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "tombstone_indices"), "set_tombstone_indices", "get_tombstone_indices");
}

SoAVisualizer::SoAVisualizer() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

SoAVisualizer::~SoAVisualizer() {
    _clear_connections();
}

void SoAVisualizer::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void SoAVisualizer::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    hovered_element_index = -1;
    hovered_ribbon_index = -1;
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

void SoAVisualizer::_on_buffer_changed() {
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

Color SoAVisualizer::_get_type_color(DataType p_type) const {
    switch (p_type) {
        case DataType::BOOL:
            return has_theme_color("type_bool", "MemoryBufferTheme") 
                 ? get_theme_color("type_bool", "MemoryBufferTheme") 
                 : Color(0.96f, 0.62f, 0.04f);
        case DataType::BYTE:
            return has_theme_color("type_byte", "MemoryBufferTheme") 
                 ? get_theme_color("type_byte", "MemoryBufferTheme") 
                 : Color(0.47f, 0.44f, 0.42f);
        case DataType::INT32:
            return has_theme_color("type_int32", "MemoryBufferTheme") 
                 ? get_theme_color("type_int32", "MemoryBufferTheme") 
                 : Color(0.06f, 0.72f, 0.51f);
        case DataType::INT64:
            return has_theme_color("type_int64", "MemoryBufferTheme") 
                 ? get_theme_color("type_int64", "MemoryBufferTheme") 
                 : Color(0.02f, 0.47f, 0.34f);
        case DataType::FLOAT32:
            return has_theme_color("type_float32", "MemoryBufferTheme") 
                 ? get_theme_color("type_float32", "MemoryBufferTheme") 
                 : Color(0.05f, 0.65f, 0.91f);
        case DataType::FLOAT64:
            return has_theme_color("type_float64", "MemoryBufferTheme") 
                 ? get_theme_color("type_float64", "MemoryBufferTheme") 
                 : Color(0.11f, 0.31f, 0.85f);
        case DataType::VECTOR2:
        case DataType::VECTOR3:
        case DataType::VECTOR4:
            return has_theme_color("type_vec_float", "MemoryBufferTheme") 
                 ? get_theme_color("type_vec_float", "MemoryBufferTheme") 
                 : Color(0.02f, 0.71f, 0.83f);
        case DataType::VECTOR2I:
        case DataType::VECTOR3I:
        case DataType::VECTOR4I:
            return has_theme_color("type_vec_int", "MemoryBufferTheme") 
                 ? get_theme_color("type_vec_int", "MemoryBufferTheme") 
                 : Color(0.08f, 0.72f, 0.65f);
        case DataType::VECTOR2D:
        case DataType::VECTOR3D:
        case DataType::VECTOR4D:
            return has_theme_color("type_vec_double", "MemoryBufferTheme") 
                 ? get_theme_color("type_vec_double", "MemoryBufferTheme") 
                 : Color(0.26f, 0.22f, 0.79f);
        case DataType::COLOR:
            return has_theme_color("type_color", "MemoryBufferTheme") 
                 ? get_theme_color("type_color", "MemoryBufferTheme") 
                 : Color(0.85f, 0.27f, 0.94f);
        case DataType::CUSTOM:
            return has_theme_color("type_custom", "MemoryBufferTheme") 
                 ? get_theme_color("type_custom", "MemoryBufferTheme") 
                 : Color(0.55f, 0.36f, 0.96f);
        default:
            return Color(0.35f, 0.35f, 0.38f);
    }
}

String SoAVisualizer::_get_type_badge(DataType p_type) const {
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

String SoAVisualizer::_get_type_name(DataType p_type) const {
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

void SoAVisualizer::_recalculate_layout() {
    cached_ribbons.clear();
    if (!active_buffer.is_valid()) return;

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    int max_elements = active_buffer->get_max_elements();
    if (cols.is_empty() || max_elements <= 0) return;

    size_t current_offset = 0;
    for (int i = 0; i < cols.size(); ++i) {
        Dictionary col = cols[i];
        RibbonMetadata r;
        r.name = col.has("name") ? String(col["name"]) : "stream_" + String::num_int64(i);
        int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);
        r.data_type = static_cast<DataType>(raw_dt);
        r.col_size = col.has("size") ? static_cast<int>(col["size"]) : 4;
        r.alignment = col.has("alignment") ? static_cast<int>(col["alignment"]) : r.col_size;

        current_offset = ideam::core::MemoryUtilities::align_to(current_offset, 64);
        r.stream_offset_bytes = current_offset;
        r.total_stream_bytes = static_cast<size_t>(max_elements) * static_cast<size_t>(r.col_size);
        current_offset += r.total_stream_bytes;

        cached_ribbons.push_back(r);
    }
}

bool SoAVisualizer::_is_element_dead(int p_index, int p_effective_active) const {
    if (p_index >= p_effective_active) return false;

    if (!tombstone_indices.is_empty()) {
        return tombstone_indices.has(p_index);
    }

    if (dead_element_count > 0) {
        // Form a trailing contiguous death row within the active window
        int death_row_start = Math::max(0, p_effective_active - dead_element_count);
        return (p_index >= death_row_start);
    }

    return false;
}

void SoAVisualizer::_draw_crosshatch(const Rect2& p_rect, const Color& p_line_color, float p_spacing) {
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

void SoAVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_element_index != -1 || hovered_ribbon_index != -1) {
            hovered_element_index = -1;
            hovered_ribbon_index = -1;
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 12;

        if (!active_buffer.is_valid() || cached_ribbons.is_empty()) {
            Color placeholder_col = Color(0.5f, 0.5f, 0.55f, 0.8f);
            String msg = "No SoA streams defined. Add columns in the Column Panel.";
            if (font.is_valid()) {
                Vector2 text_sz = font->get_string_size(msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                draw_string(font, (size - text_sz) / 2.0f + Vector2(0, text_sz.y), msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, placeholder_col);
            }
            return;
        }

        int max_elements = active_buffer->get_max_elements();
        if (max_elements <= 0) return;

        int effective_active = (active_element_count >= 0) 
                             ? Math::min(active_element_count, max_elements) 
                             : max_elements;

        int num_ribbons = cached_ribbons.size();

        // Layout Metrics
        float header_h = 24.0f;
        float footer_h = 22.0f;
        float spacing = 6.0f;
        float total_spacing = spacing * (num_ribbons - 1);
        float available_ribbon_h = size.y - header_h - footer_h - total_spacing;
        float ribbon_h = available_ribbon_h / static_cast<float>(num_ribbons);
        if (ribbon_h < 24.0f) ribbon_h = 24.0f;

        // Palette Tokens
        Color text_primary = has_theme_color("text_color", "SoAVisualizer") 
                           ? get_theme_color("text_color", "SoAVisualizer") 
                           : Color(0.95f, 0.95f, 0.95f, 1.0f);
        Color text_muted = Color(0.65f, 0.65f, 0.70f, 0.9f);
        Color border_col = Color(0.12f, 0.12f, 0.15f, 1.0f);
        Color unallocated_col = Color(0.15f, 0.15f, 0.17f, 1.0f);
        Color dead_space_col = Color(0.85f, 0.22f, 0.22f, 0.85f);
        Color dead_cross_col = Color(0.35f, 0.10f, 0.10f, 0.90f);
        Color cache_boundary_col = Color(0.95f, 0.75f, 0.15f, 0.80f); // High-contrast gold
        Color simd_boundary_col = Color(1.0f, 1.0f, 1.0f, 0.22f);     // Subtle semi-transparent white
        Color hover_cursor_col = Color(1.0f, 1.0f, 1.0f, 0.65f);

        // --- 1. Top Telemetry Header ---
        if (font.is_valid()) {
            String title_str = String("PARALLEL STREAMS (SOA): ") + String::num_int64(num_ribbons) + " Streams | " +
                               String::num_int64(max_elements) + " Max Elements | 64B Cache-Aligned";
            draw_string(font, Vector2(0.0f, header_h - 7.0f), title_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_primary);

            if (needs_compaction || dead_element_count > 0 || !tombstone_indices.is_empty()) {
                int tombstones = tombstone_indices.is_empty() ? dead_element_count : tombstone_indices.size();
                String comp_str = String("[DEATH ROW / COMPACTION REQUIRED: ") + String::num_int64(tombstones) + " Dead Slots]";
                Vector2 comp_sz = font->get_string_size(comp_str, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size);
                draw_string(font, Vector2(size.x - comp_sz.x, header_h - 7.0f), comp_str, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, Color(0.95f, 0.35f, 0.35f, 1.0f));
            }
        }

        float pixels_per_element = size.x / static_cast<float>(max_elements);
        float cur_y = header_h;

        // --- 2. Stacked Ribbons ---
        for (int r = 0; r < num_ribbons; ++r) {
            RibbonMetadata& meta = cached_ribbons.write[r];
            meta.rect = Rect2(0.0f, cur_y, size.x, ribbon_h);

            // A. Base Stream Fill (Active vs Headroom)
            float active_w = effective_active * pixels_per_element;
            Color type_col = _get_type_color(meta.data_type);

            if (active_w > 0.0f) {
                draw_rect(Rect2(0.0f, cur_y, active_w, ribbon_h), type_col);
            }

            if (active_w < size.x) {
                Rect2 headroom_rect(active_w, cur_y, size.x - active_w, ribbon_h);
                draw_rect(headroom_rect, unallocated_col);
                _draw_crosshatch(headroom_rect, Color(0.22f, 0.22f, 0.26f, 0.6f), 10.0f);
            }

            // B. Tombstone / Compaction Dead Space Overlays
            if (tombstone_indices.size() > 0) {
                for (int t = 0; t < tombstone_indices.size(); ++t) {
                    int dead_idx = tombstone_indices[t];
                    if (dead_idx >= 0 && dead_idx < effective_active) {
                        float dx = dead_idx * pixels_per_element;
                        float dw = Math::max(pixels_per_element, 2.0f);
                        Rect2 dead_rect(dx, cur_y, dw, ribbon_h);
                        draw_rect(dead_rect, dead_space_col);
                        if (dw > 8.0f) {
                            _draw_crosshatch(dead_rect, dead_cross_col, 4.0f);
                        }
                    }
                }
            } else if (dead_element_count > 0) {
                int dead_start = Math::max(0, effective_active - dead_element_count);
                float dx = dead_start * pixels_per_element;
                float dw = (effective_active - dead_start) * pixels_per_element;
                if (dw > 0.0f) {
                    Rect2 dead_rect(dx, cur_y, dw, ribbon_h);
                    draw_rect(dead_rect, dead_space_col);
                    _draw_crosshatch(dead_rect, dead_cross_col, 5.0f);
                }
            }

            // C. Boundaries (Cache-lines & SIMD Lanes)
            int align_bytes = active_buffer->get_alignment() > 0 ? active_buffer->get_alignment() : 64;
            int elems_per_cache_line = Math::max(1, align_bytes / meta.col_size);
            int elems_per_simd_lane = Math::max(1, 16 / meta.col_size); // 16-byte SSE/NEON baseline

            // Draw SIMD ticks (subtle, clamped at 600 ticks)
            int simd_count = max_elements / elems_per_simd_lane;
            if (simd_count < 600 && pixels_per_element * elems_per_simd_lane >= 4.0f) {
                float simd_step = pixels_per_element * elems_per_simd_lane;
                for (float sx = simd_step; sx < size.x; sx += simd_step) {
                    draw_line(Vector2(sx, cur_y), Vector2(sx, cur_y + 4.0f), simd_boundary_col, 1.0f);
                    draw_line(Vector2(sx, cur_y + ribbon_h - 4.0f), Vector2(sx, cur_y + ribbon_h), simd_boundary_col, 1.0f);
                }
            }

            // Draw 64B Cache Boundaries (high contrast, clamped at 1000 lines)
            int cache_count = max_elements / elems_per_cache_line;
            if (cache_count < 1000 && pixels_per_element * elems_per_cache_line >= 3.0f) {
                float cache_step = pixels_per_element * elems_per_cache_line;
                for (float cx = cache_step; cx < size.x; cx += cache_step) {
                    draw_line(Vector2(cx, cur_y), Vector2(cx, cur_y + ribbon_h), cache_boundary_col, 1.0f);
                }
            }

            // D. Ribbon Border
            draw_rect(meta.rect, border_col, false, 1.5f);

            // E. Identification Label Overlay
            if (font.is_valid()) {
                String badge = _get_type_badge(meta.data_type);
                String label_text = badge + " " + meta.name + " (" + String::num_int64(meta.col_size) + "B/elem) " +
                                    "| 64B Cache = " + String::num_int64(elems_per_cache_line) + " elems";

                Vector2 str_sz = font->get_string_size(label_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 1);
                float pill_w = str_sz.x + 16.0f;
                Rect2 pill_rect(cur_y > header_h ? 2.0f : 2.0f, cur_y + 2.0f, pill_w, ribbon_h - 4.0f);

                // Translucent pill backdrop for readability over lines
                draw_rect(pill_rect, Color(0.08f, 0.08f, 0.10f, 0.82f));
                draw_rect(pill_rect, border_col, false, 1.0f);

                Vector2 text_pos(pill_rect.position.x + 8.0f, cur_y + (ribbon_h * 0.5f) + ((font_size - 1) * 0.35f));
                draw_string(font, text_pos + Vector2(1, 1), label_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 1, Color(0, 0, 0, 0.95f));
                draw_string(font, text_pos, label_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 1, text_primary);
            }

            cur_y += ribbon_h + spacing;
        }

        // --- 3. Synchronized Multi-Stream Cursor (Vertical Across All Ribbons) ---
        if (hovered_element_index >= 0 && hovered_element_index < max_elements) {
            float cursor_x = hovered_element_index * pixels_per_element;
            float cursor_w = Math::max(pixels_per_element, 2.0f);

            float all_ribbons_y = header_h;
            float all_ribbons_h = size.y - header_h - footer_h;

            // Highlight column segment across all parallel ribbons
            draw_rect(Rect2(cursor_x, all_ribbons_y, cursor_w, all_ribbons_h), hover_cursor_col);

            // Emphasize the specific cell hovered on the focused ribbon
            if (hovered_ribbon_index >= 0 && hovered_ribbon_index < num_ribbons) {
                const RibbonMetadata& r_meta = cached_ribbons[hovered_ribbon_index];
                draw_rect(Rect2(cursor_x, r_meta.rect.position.y, cursor_w, r_meta.rect.size.y), Color(1.0f, 0.85f, 0.25f, 0.9f), false, 2.0f);
            }
        }

        // --- 4. Bottom Footer Telemetry ---
        if (font.is_valid()) {
            float total_soa_bytes = active_buffer->calculate_raw_data_bytes();
            String footer_str;

            if (hovered_element_index >= 0) {
                bool is_dead = _is_element_dead(hovered_element_index, effective_active);
                bool is_headroom = (hovered_element_index >= effective_active);
                String state_str = is_headroom ? "UNALLOCATED HEADROOM" : (is_dead ? "TOMBSTONE (DEAD)" : "ACTIVE");

                footer_str = String("Selected Entity: [") + String::num_int64(hovered_element_index) + " / " + String::num_int64(max_elements - 1) + "] " +
                             "| State: " + state_str + " | Total SoA Footprint: " + String::num_uint64(total_soa_bytes) + " Bytes";
            } else {
                footer_str = String("Total SoA Raw Allocation: ") + String::num_uint64(total_soa_bytes) + " Bytes (" +
                             String::num_real(static_cast<double>(total_soa_bytes) / 1024.0, false) + " KB)";
            }

            draw_string(font, Vector2(0.0f, size.y - 6.0f), footer_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 1, text_muted);
        }
    }
}

void SoAVisualizer::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && active_buffer.is_valid()) {
        int max_elements = active_buffer->get_max_elements();
        Vector2 size = get_size();

        if (max_elements > 0 && size.x > 0.0f) {
            Vector2 pos = mm->get_position();
            float rel_x = CLAMP(pos.x, 0.0f, size.x);
            int new_hovered_elem = CLAMP(static_cast<int>((rel_x / size.x) * static_cast<float>(max_elements)), 0, max_elements - 1);

            int new_hovered_ribbon = -1;
            for (int r = 0; r < cached_ribbons.size(); ++r) {
                if (cached_ribbons[r].rect.has_point(pos)) {
                    new_hovered_ribbon = r;
                    break;
                }
            }

            if (new_hovered_elem != hovered_element_index || new_hovered_ribbon != hovered_ribbon_index) {
                hovered_element_index = new_hovered_elem;
                hovered_ribbon_index = new_hovered_ribbon;
                queue_redraw();
            }
        }
    }
    Control::_gui_input(p_event);
}

String SoAVisualizer::_get_tooltip(const Vector2& p_at_position) const {
    if (!active_buffer.is_valid() || cached_ribbons.is_empty()) return String();

    int max_elements = active_buffer->get_max_elements();
    if (max_elements <= 0) return String();

    Vector2 size = get_size();
    if (size.x <= 0.0f) return String();

    float rel_x = CLAMP(p_at_position.x, 0.0f, size.x);
    int elem_idx = CLAMP(static_cast<int>((rel_x / size.x) * static_cast<float>(max_elements)), 0, max_elements - 1);

    int target_ribbon = -1;
    for (int r = 0; r < cached_ribbons.size(); ++r) {
        if (cached_ribbons[r].rect.has_point(p_at_position)) {
            target_ribbon = r;
            break;
        }
    }

    int effective_active = (active_element_count >= 0) 
                         ? Math::min(active_element_count, max_elements) 
                         : max_elements;

    bool is_dead = _is_element_dead(elem_idx, effective_active);
    bool is_headroom = (elem_idx >= effective_active);

    String status_desc = is_headroom ? "Unallocated Headroom" : (is_dead ? "Dead (Pending Compaction)" : "Active");

    if (target_ribbon >= 0 && target_ribbon < cached_ribbons.size()) {
        const RibbonMetadata& r = cached_ribbons[target_ribbon];
        size_t byte_in_stream = static_cast<size_t>(elem_idx) * static_cast<size_t>(r.col_size);
        size_t abs_byte_offset = r.stream_offset_bytes + byte_in_stream;

        int align_bytes = active_buffer->get_alignment() > 0 ? active_buffer->get_alignment() : 64;
        int elems_per_cache = Math::max(1, align_bytes / r.col_size);
        int cache_line_idx = elem_idx / elems_per_cache;

        return String("[SoA Parallel Stream: ") + r.name + "]\n" +
               "Data Type: " + _get_type_name(r.data_type) + " " + _get_type_badge(r.data_type) + "\n" +
               "Entity Index: [" + String::num_int64(elem_idx) + " / " + String::num_int64(max_elements - 1) + "]\n" +
               "Slot Status: " + status_desc + "\n" +
               "Element Byte Size: " + String::num_int64(r.col_size) + " Bytes\n" +
               "Stream Offset: 0x" + String::num_uint64(abs_byte_offset, 16, true) + " (" + String::num_uint64(abs_byte_offset) + " B)\n" +
               "Cache Line: #" + String::num_int64(cache_line_idx) + " (" + String::num_int64(align_bytes) + "-byte aligned)";
    }

    return String("Entity Index: [") + String::num_int64(elem_idx) + " / " + String::num_int64(max_elements - 1) + "]\n" +
           "Slot Status: " + status_desc;
}

Vector2 SoAVisualizer::_get_minimum_size() const {
    int num_ribbons = 1;
    if (active_buffer.is_valid()) {
        num_ribbons = active_buffer->get_columns().size();
        if (num_ribbons == 0) num_ribbons = 1;
    }

    float min_ribbon_h = 24.0f;
    float spacing = 6.0f;
    float header_footer = 46.0f;
    float total_h = header_footer + (min_ribbon_h * num_ribbons) + (spacing * (num_ribbons - 1));

    return Vector2(140.0f, total_h);
}

// --- Compaction Mutators ---

void SoAVisualizer::set_needs_compaction(bool p_needed) {
    if (needs_compaction == p_needed) return;
    needs_compaction = p_needed;
    queue_redraw();
}
bool SoAVisualizer::get_needs_compaction() const { return needs_compaction; }

void SoAVisualizer::set_active_element_count(int p_count) {
    if (active_element_count == p_count) return;
    active_element_count = p_count;
    queue_redraw();
}
int SoAVisualizer::get_active_element_count() const { return active_element_count; }

void SoAVisualizer::set_dead_element_count(int p_count) {
    if (dead_element_count == p_count) return;
    dead_element_count = p_count;
    queue_redraw();
}
int SoAVisualizer::get_dead_element_count() const { return dead_element_count; }

void SoAVisualizer::set_tombstone_indices(const PackedInt32Array& p_indices) {
    tombstone_indices = p_indices;
    queue_redraw();
}
PackedInt32Array SoAVisualizer::get_tombstone_indices() const { return tombstone_indices; }

} // namespace ideam::godot_ext