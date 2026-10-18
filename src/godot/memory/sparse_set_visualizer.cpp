#include "sparse_set_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void SparseSetVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &SparseSetVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &SparseSetVisualizer::_on_buffer_changed);

    ClassDB::bind_method(D_METHOD("set_simulated_dense_count", "count"), &SparseSetVisualizer::set_simulated_dense_count);
    ClassDB::bind_method(D_METHOD("get_simulated_dense_count"), &SparseSetVisualizer::get_simulated_dense_count);

    ClassDB::bind_method(D_METHOD("set_dense_to_entity_map", "map"), &SparseSetVisualizer::set_dense_to_entity_map);
    ClassDB::bind_method(D_METHOD("get_dense_to_entity_map"), &SparseSetVisualizer::get_dense_to_entity_map);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "simulated_dense_count"), "set_simulated_dense_count", "get_simulated_dense_count");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "dense_to_entity_map"), "set_dense_to_entity_map", "get_dense_to_entity_map");
}

SparseSetVisualizer::SparseSetVisualizer() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

SparseSetVisualizer::~SparseSetVisualizer() {
    _clear_connections();
}

void SparseSetVisualizer::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void SparseSetVisualizer::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    hovered_tier = -1;
    hovered_entity_id = -1;
    hovered_dense_index = -1;

    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

void SparseSetVisualizer::_on_buffer_changed() {
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

Color SparseSetVisualizer::_get_type_color(DataType p_type) const {
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

String SparseSetVisualizer::_get_type_badge(DataType p_type) const {
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

String SparseSetVisualizer::_get_type_name(DataType p_type) const {
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

void SparseSetVisualizer::_ensure_simulation_mapping() {
    if (!active_buffer.is_valid()) return;

    int64_t universe = active_buffer->get_sparse_universe_size();
    int max_dense = active_buffer->get_max_elements();
    if (universe <= 0 || max_dense <= 0) return;

    simulated_dense_count = CLAMP(simulated_dense_count, 1, max_dense);

    if (dense_to_entity_map.size() != simulated_dense_count) {
        dense_to_entity_map.resize(simulated_dense_count);
        // Generate pseudo-random scattered entity IDs across the universe
        for (int i = 0; i < simulated_dense_count; ++i) {
            int64_t salt = (static_cast<int64_t>(i) * 2654435761ULL) ^ 0x5DEECE66DULL;
            int64_t entity_id = Math::abs(salt) % universe;
            dense_to_entity_map.set(i, entity_id);
        }
    }
}

int SparseSetVisualizer::_find_dense_for_entity(int64_t p_entity_id) const {
    for (int i = 0; i < dense_to_entity_map.size(); ++i) {
        if (dense_to_entity_map[i] == p_entity_id) {
            return i;
        }
    }
    return -1;
}

int64_t SparseSetVisualizer::_get_entity_for_dense(int p_dense_idx) const {
    if (p_dense_idx >= 0 && p_dense_idx < dense_to_entity_map.size()) {
        return dense_to_entity_map[p_dense_idx];
    }
    return -1;
}

void SparseSetVisualizer::_recalculate_layout() {
    cached_columns.clear();
    if (!active_buffer.is_valid()) return;

    _ensure_simulation_mapping();

    TypedArray<Dictionary> cols = active_buffer->get_columns();
    int max_elements = active_buffer->get_max_elements();
    if (max_elements <= 0) return;

    // 1. Entity ID reverse-lookup stream is implicitly the primary dense column in SparseSet
    DenseColumnInfo entity_id_col;
    entity_id_col.name = "entity_id";
    entity_id_col.data_type = DataType::INT64;
    entity_id_col.col_size = sizeof(uint32_t); // Standard dense array holds uint32_t entity back-refs
    entity_id_col.alignment = 4;
    entity_id_col.stream_offset = 0;
    entity_id_col.stream_bytes = static_cast<size_t>(max_elements) * sizeof(uint32_t);
    cached_columns.push_back(entity_id_col);

    size_t current_offset = ideam::core::MemoryUtilities::align_to(entity_id_col.stream_bytes, 64);

    // 2. Component Pools
    for (int i = 0; i < cols.size(); ++i) {
        Dictionary col = cols[i];
        DenseColumnInfo ci;
        ci.name = col.has("name") ? String(col["name"]) : "component_" + String::num_int64(i);
        int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);
        ci.data_type = static_cast<DataType>(raw_dt);
        ci.col_size = col.has("size") ? static_cast<int>(col["size"]) : 4;
        ci.alignment = col.has("alignment") ? static_cast<int>(col["alignment"]) : ci.col_size;

        current_offset = ideam::core::MemoryUtilities::align_to(current_offset, 64);
        ci.stream_offset = current_offset;
        ci.stream_bytes = static_cast<size_t>(max_elements) * static_cast<size_t>(ci.col_size);
        current_offset += ci.stream_bytes;

        cached_columns.push_back(ci);
    }
}

void SparseSetVisualizer::_draw_bezier_link(const Vector2& p_start, const Vector2& p_end, const Color& p_color, float p_width) {
    float mid_y = (p_start.y + p_end.y) * 0.5f;
    Vector2 cp1(p_start.x, mid_y);
    Vector2 cp2(p_end.x, mid_y);

    int segments = 24;
    Vector2 prev = p_start;

    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float u = 1.0f - t;
        Vector2 pt = (u * u * u * p_start) + (3.0f * u * u * t * cp1) + (3.0f * u * t * t * cp2) + (t * t * t * p_end);
        draw_line(prev, pt, p_color, p_width, true);
        prev = pt;
    }
}

void SparseSetVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_tier != -1 || hovered_entity_id != -1 || hovered_dense_index != -1) {
            hovered_tier = -1;
            hovered_entity_id = -1;
            hovered_dense_index = -1;
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 12;

        if (!active_buffer.is_valid()) {
            Color placeholder_col = Color(0.5f, 0.5f, 0.55f, 0.8f);
            String msg = "No Sparse Set Buffer bound.";
            if (font.is_valid()) {
                Vector2 text_sz = font->get_string_size(msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                draw_string(font, (size - text_sz) / 2.0f + Vector2(0, text_sz.y), msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, placeholder_col);
            }
            return;
        }

        int64_t universe_size = active_buffer->get_sparse_universe_size();
        int max_dense = active_buffer->get_max_elements();
        if (universe_size <= 0 || max_dense <= 0) return;

        // Palette
        Color text_primary = Color(0.95f, 0.95f, 0.95f, 1.0f);
        Color text_muted = Color(0.65f, 0.65f, 0.70f, 0.9f);
        Color border_col = Color(0.12f, 0.12f, 0.15f, 1.0f);
        Color sparse_bg = Color(0.11f, 0.11f, 0.14f, 1.0f);
        Color sparse_mapped_col = Color(0.96f, 0.62f, 0.04f, 0.95f); // Amber / Gold indicator
        Color trace_color = Color(0.66f, 0.33f, 0.97f, 1.0f);        // Electric Violet
        Color unallocated_col = Color(0.15f, 0.15f, 0.17f, 1.0f);

        // Vertical Section Metrics
        float header_h = 24.0f;
        float sparse_track_h = 32.0f;
        float trace_h = 44.0f;
        float dense_ribbon_h = 22.0f;
        float dense_spacing = 4.0f;
        int num_dense_ribbons = cached_columns.size();
        float footer_h = 22.0f;

        float cur_y = 0.0f;

        // --- 1. Top Telemetry Header ---
        if (font.is_valid()) {
            size_t sparse_bytes = ideam::core::MemoryUtilities::align_to(universe_size * sizeof(uint32_t), 64);
            String hdr = String("SPARSE SET ARCHITECTURE: Universe = ") + String::num_int64(universe_size) +
                         " Entities (" + String::num_uint64(sparse_bytes) + " B) -> Dense Capacity = " +
                         String::num_int64(max_dense) + " Slots";
            draw_string(font, Vector2(0.0f, header_h - 7.0f), hdr, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_primary);
        }
        cur_y += header_h;

        // --- 2. Tier 1: Sparse Index Array (Upper) ---
        sparse_tier_rect = Rect2(0.0f, cur_y, size.x, sparse_track_h);
        draw_rect(sparse_tier_rect, sparse_bg);

        float pixels_per_universe_elem = size.x / static_cast<float>(universe_size);

        // Draw mapped entity notches in Sparse Array
        for (int i = 0; i < dense_to_entity_map.size(); ++i) {
            int64_t mapped_ent = dense_to_entity_map[i];
            if (mapped_ent >= 0 && mapped_ent < universe_size) {
                float ex = static_cast<float>(mapped_ent) * pixels_per_universe_elem;
                float ew = Math::max(pixels_per_universe_elem, 2.5f);
                draw_rect(Rect2(ex, cur_y + 2.0f, ew, sparse_track_h - 4.0f), sparse_mapped_col);
            }
        }

        draw_rect(sparse_tier_rect, border_col, false, 1.5f);

        if (font.is_valid()) {
            String sparse_lbl = String("SPARSE ARRAY: [Entity IDs 0 .. ") + String::num_int64(universe_size - 1) + "]";
            draw_string(font, Vector2(8.0f, cur_y + (sparse_track_h * 0.65f)), sparse_lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
        }
        cur_y += sparse_track_h;

        // --- 3. Middle Tracing Channel Area ---
        trace_channel_y = cur_y;
        trace_channel_height = trace_h;
        cur_y += trace_h;

        // --- 4. Tier 2: Packed Dense Component Pools (Lower) ---
        float dense_y_start = cur_y;
        float pixels_per_dense_elem = size.x / static_cast<float>(max_dense);
        float active_dense_w = simulated_dense_count * pixels_per_dense_elem;

        for (int c = 0; c < num_dense_ribbons; ++c) {
            DenseColumnInfo& ci = cached_columns.write[c];
            ci.ribbon_rect = Rect2(0.0f, cur_y, size.x, dense_ribbon_h);

            // Active dense slots
            Color col_color = _get_type_color(ci.data_type);
            if (active_dense_w > 0.0f) {
                draw_rect(Rect2(0.0f, cur_y, active_dense_w, dense_ribbon_h), col_color);
            }

            // Headroom slots beyond active
            if (active_dense_w < size.x) {
                draw_rect(Rect2(active_dense_w, cur_y, size.x - active_dense_w, dense_ribbon_h), unallocated_col);
            }

            // Dense slot separator lines
            if (pixels_per_dense_elem >= 4.0f) {
                for (int d = 1; d < max_dense; ++d) {
                    float dx = d * pixels_per_dense_elem;
                    Color sep_col = (d == simulated_dense_count) ? Color(1, 1, 1, 0.6f) : Color(0, 0, 0, 0.35f);
                    draw_line(Vector2(dx, cur_y), Vector2(dx, cur_y + dense_ribbon_h), sep_col, 1.0f);
                }
            }

            draw_rect(ci.ribbon_rect, border_col, false, 1.0f);

            // Column Name Label
            if (font.is_valid()) {
                String badge = _get_type_badge(ci.data_type);
                String c_label = badge + " " + ci.name + " (" + String::num_int64(ci.col_size) + "B/elem)";
                Vector2 str_sz = font->get_string_size(c_label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2);

                Rect2 pill(2.0f, cur_y + 1.0f, str_sz.x + 8.0f, dense_ribbon_h - 2.0f);
                draw_rect(pill, Color(0.08f, 0.08f, 0.10f, 0.85f));

                draw_string(font, Vector2(6.0f, cur_y + (dense_ribbon_h * 0.7f)), c_label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_primary);
            }

            cur_y += dense_ribbon_h + dense_spacing;
        }

        dense_tier_rect = Rect2(0.0f, dense_y_start, size.x, cur_y - dense_y_start);

        // --- 5. Interactive Link Tracing Connection ---
        int64_t target_ent = -1;
        int target_dense = -1;

        if (hovered_tier == 0 && hovered_entity_id >= 0) {
            target_ent = hovered_entity_id;
            target_dense = _find_dense_for_entity(target_ent);
        } else if (hovered_tier == 1 && hovered_dense_index >= 0) {
            target_dense = hovered_dense_index;
            target_ent = _get_entity_for_dense(target_dense);
        }

        if (target_ent >= 0) {
            float sx = static_cast<float>(target_ent) * pixels_per_universe_elem;
            float sw = Math::max(pixels_per_universe_elem, 3.0f);
            Vector2 sparse_pt(sx + (sw * 0.5f), sparse_tier_rect.position.y + sparse_track_h);

            // Highlight Sparse Cell
            draw_rect(Rect2(sx, sparse_tier_rect.position.y, sw, sparse_track_h), trace_color);
            draw_rect(Rect2(sx - 1.0f, sparse_tier_rect.position.y - 1.0f, sw + 2.0f, sparse_track_h + 2.0f), Color(1, 1, 1, 0.9f), false, 1.5f);

            if (target_dense >= 0 && target_dense < max_dense) {
                float dx = target_dense * pixels_per_dense_elem;
                float dw = Math::max(pixels_per_dense_elem, 3.0f);
                Vector2 dense_pt(dx + (dw * 0.5f), dense_tier_rect.position.y);

                // Highlight Dense Slot Across Ribbons
                draw_rect(Rect2(dx, dense_tier_rect.position.y, dw, dense_tier_rect.size.y), Color(trace_color.r, trace_color.g, trace_color.b, 0.45f));
                draw_rect(Rect2(dx, dense_tier_rect.position.y, dw, dense_tier_rect.size.y), trace_color, false, 2.0f);

                // Draw Bezier Link
                _draw_bezier_link(sparse_pt, dense_pt, trace_color, 2.5f);

                // Midpoint Connection Badge
                if (font.is_valid()) {
                    Vector2 mid_pt = (sparse_pt + dense_pt) * 0.5f;
                    String badge_text = String("Entity #") + String::num_int64(target_ent) + " ➔ Dense Slot #" + String::num_int64(target_dense);
                    Vector2 b_sz = font->get_string_size(badge_text, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2);

                    Rect2 b_rect(mid_pt.x - (b_sz.x / 2.0f) - 6.0f, mid_pt.y - 10.0f, b_sz.x + 12.0f, 20.0f);
                    draw_rect(b_rect, Color(0.12f, 0.10f, 0.16f, 0.92f));
                    draw_rect(b_rect, trace_color, false, 1.0f);
                    draw_string(font, Vector2(b_rect.position.x + 6.0f, b_rect.position.y + 14.0f), badge_text, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, text_primary);
                }
            } else {
                // Dangling unmapped link indicator
                Vector2 drop_pt(sparse_pt.x, sparse_pt.y + 16.0f);
                draw_line(sparse_pt, drop_pt, Color(0.9f, 0.3f, 0.3f, 0.8f), 2.0f);
                if (font.is_valid()) {
                    String unmapped_str = "TOMBSTONE / UNMAPPED";
                    draw_string(font, drop_pt + Vector2(-40.0f, 14.0f), unmapped_str, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 3, Color(0.9f, 0.4f, 0.4f, 0.9f));
                }
            }
        }

        // --- 6. Footer Telemetry ---
        if (font.is_valid()) {
            int total_sparse_footprint = active_buffer->calculate_raw_data_bytes();
            String foot_str = "Total Footprint: " + String::num_uint64(total_sparse_footprint) + " Bytes (" +
                              String::num_real(static_cast<double>(total_sparse_footprint) / 1024.0, false) + " KB) | Hover to inspect pointer mapping.";
            draw_string(font, Vector2(0.0f, size.y - 6.0f), foot_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
        }
    }
}

void SparseSetVisualizer::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && active_buffer.is_valid()) {
        Vector2 pos = mm->get_position();
        Vector2 size = get_size();

        int64_t universe_size = active_buffer->get_sparse_universe_size();
        int max_dense = active_buffer->get_max_elements();

        int new_hovered_tier = -1;
        int64_t new_hovered_entity = -1;
        int new_hovered_dense = -1;

        if (sparse_tier_rect.has_point(pos) && universe_size > 0 && size.x > 0.0f) {
            new_hovered_tier = 0;
            float rel_x = CLAMP(pos.x, 0.0f, size.x);
            new_hovered_entity = CLAMP(static_cast<int64_t>((rel_x / size.x) * static_cast<float>(universe_size)), 0, universe_size - 1);
        } else if (dense_tier_rect.has_point(pos) && max_dense > 0 && size.x > 0.0f) {
            new_hovered_tier = 1;
            float rel_x = CLAMP(pos.x, 0.0f, size.x);
            new_hovered_dense = CLAMP(static_cast<int>((rel_x / size.x) * static_cast<float>(max_dense)), 0, max_dense - 1);
        }

        if (new_hovered_tier != hovered_tier || new_hovered_entity != hovered_entity_id || new_hovered_dense != hovered_dense_index) {
            hovered_tier = new_hovered_tier;
            hovered_entity_id = new_hovered_entity;
            hovered_dense_index = new_hovered_dense;
            queue_redraw();
        }
    }

    Control::_gui_input(p_event);
}

String SparseSetVisualizer::_get_tooltip(const Vector2& p_at_position) const {
    if (!active_buffer.is_valid()) return String();

    int64_t universe_size = active_buffer->get_sparse_universe_size();
    int max_dense = active_buffer->get_max_elements();
    Vector2 size = get_size();
    if (size.x <= 0.0f) return String();

    if (sparse_tier_rect.has_point(p_at_position) && universe_size > 0) {
        float rel_x = CLAMP(p_at_position.x, 0.0f, size.x);
        int64_t entity_id = CLAMP(static_cast<int64_t>((rel_x / size.x) * static_cast<float>(universe_size)), 0, universe_size - 1);
        int dense_idx = _find_dense_for_entity(entity_id);
        size_t byte_off = static_cast<size_t>(entity_id) * sizeof(uint32_t);

        if (dense_idx != -1) {
            return String("[Sparse Array Entry: Entity #") + String::num_int64(entity_id) + "]\n" +
                   "Status: Mapped (Active)\n" +
                   "Dense Index Pointer: #" + String::num_int64(dense_idx) + "\n" +
                   "Table Offset: 0x" + String::num_uint64(byte_off, 16, true) + " (" + String::num_uint64(byte_off) + " B)";
        } else {
            return String("[Sparse Array Entry: Entity #") + String::num_int64(entity_id) + "]\n" +
                   "Status: Unmapped / Tombstone (0xFFFFFFFF)\n" +
                   "Table Offset: 0x" + String::num_uint64(byte_off, 16, true) + " (" + String::num_uint64(byte_off) + " B)";
        }
    } else if (dense_tier_rect.has_point(p_at_position) && max_dense > 0) {
        float rel_x = CLAMP(p_at_position.x, 0.0f, size.x);
        int dense_idx = CLAMP(static_cast<int>((rel_x / size.x) * static_cast<float>(max_dense)), 0, max_dense - 1);
        int64_t mapped_entity = _get_entity_for_dense(dense_idx);

        if (dense_idx < simulated_dense_count && mapped_entity != -1) {
            return String("[Packed Dense Slot: #") + String::num_int64(dense_idx) + "]\n" +
                   "Mapped Entity ID: #" + String::num_int64(mapped_entity) + "\n" +
                   "Payload: " + String::num_int64(cached_columns.size()) + " parallel component streams\n" +
                   "Status: Active Component Slot";
        } else {
            return String("[Packed Dense Slot: #") + String::num_int64(dense_idx) + "]\n" +
                   "Status: Unallocated Headroom (Capacity: " + String::num_int64(max_dense) + " max)";
        }
    }

    return Control::_get_tooltip(p_at_position);
}

void SparseSetVisualizer::set_simulated_dense_count(int p_count) {
    if (simulated_dense_count == p_count) return;
    simulated_dense_count = p_count;
    _ensure_simulation_mapping();
    queue_redraw();
}
int SparseSetVisualizer::get_simulated_dense_count() const { return simulated_dense_count; }

void SparseSetVisualizer::set_dense_to_entity_map(const PackedInt64Array& p_map) {
    dense_to_entity_map = p_map;
    simulated_dense_count = p_map.size();
    queue_redraw();
}
PackedInt64Array SparseSetVisualizer::get_dense_to_entity_map() const { return dense_to_entity_map; }

Vector2 SparseSetVisualizer::_get_minimum_size() const {
    int num_cols = cached_columns.size();
    float dense_h = (num_cols * 22.0f) + ((num_cols - 1) * 4.0f);
    float total_h = 24.0f + 32.0f + 44.0f + dense_h + 22.0f;
    return Vector2(160.0f, total_h);
}

} // namespace ideam::godot_ext