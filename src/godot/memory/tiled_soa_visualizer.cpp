#include "tiled_soa_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
using namespace godot;

namespace ideam::godot_ext {

void TiledSoAVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &TiledSoAVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &TiledSoAVisualizer::_on_buffer_changed);

    ClassDB::bind_method(D_METHOD("set_focused_tile_index", "index"), &TiledSoAVisualizer::set_focused_tile_index);
    ClassDB::bind_method(D_METHOD("get_focused_tile_index"), &TiledSoAVisualizer::get_focused_tile_index);

    ClassDB::bind_method(D_METHOD("set_show_locality_comparison", "show"), &TiledSoAVisualizer::set_show_locality_comparison);
    ClassDB::bind_method(D_METHOD("get_show_locality_comparison"), &TiledSoAVisualizer::get_show_locality_comparison);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "focused_tile_index"), "set_focused_tile_index", "get_focused_tile_index");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_locality_comparison"), "set_show_locality_comparison", "get_show_locality_comparison");
}

TiledSoAVisualizer::TiledSoAVisualizer() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

TiledSoAVisualizer::~TiledSoAVisualizer() {
    _clear_connections();
}

void TiledSoAVisualizer::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void TiledSoAVisualizer::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    focused_tile_index = 0;
    hovered_tile_index = -1;
    hovered_sub_block_index = -1;
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

void TiledSoAVisualizer::_on_buffer_changed() {
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

Color TiledSoAVisualizer::_get_type_color(DataType p_type) const {
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

String TiledSoAVisualizer::_get_type_badge(DataType p_type) const {
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

String TiledSoAVisualizer::_get_type_name(DataType p_type) const {
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

void TiledSoAVisualizer::_recalculate_layout() {
    cached_tiles.clear();
    cached_sub_blocks.clear();
    single_tile_stride = 0;
    single_tile_raw = 0;
    single_tile_padding = 0;

    if (!active_buffer.is_valid()) return;

    int max_elements = active_buffer->get_max_elements();
    int elements_per_tile = active_buffer->get_elements_per_tile();
    TypedArray<Dictionary> cols = active_buffer->get_columns();

    if (max_elements <= 0 || elements_per_tile <= 0 || cols.is_empty()) return;

    // 1. Calculate Single-Tile Internal Sub-Blocks & Offsets
    size_t current_offset = 0;
    for (int i = 0; i < cols.size(); ++i) {
        Dictionary col = cols[i];
        int col_size = col.has("size") ? static_cast<int>(col["size"]) : 4;
        int col_align = col.has("alignment") ? static_cast<int>(col["alignment"]) : col_size;
        String name = col.has("name") ? String(col["name"]) : "col_" + String::num_int64(i);
        int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);

        size_t aligned_offset = ideam::core::MemoryUtilities::align_to(current_offset, col_align);
        int pad = static_cast<int>(aligned_offset - current_offset);

        if (pad > 0) {
            SubBlockInfo pad_block;
            pad_block.name = "Alignment Pad";
            pad_block.intra_offset = current_offset;
            pad_block.byte_size = pad;
            pad_block.is_padding = true;
            cached_sub_blocks.push_back(pad_block);
            current_offset = aligned_offset;
        }

        SubBlockInfo sub;
        sub.name = name;
        sub.data_type = static_cast<DataType>(raw_dt);
        sub.col_size = col_size;
        sub.alignment = col_align;
        sub.intra_offset = current_offset;
        sub.byte_size = static_cast<size_t>(elements_per_tile) * static_cast<size_t>(col_size);
        sub.is_padding = false;
        cached_sub_blocks.push_back(sub);

        current_offset += sub.byte_size;
    }

    single_tile_raw = current_offset;
    single_tile_stride = ideam::core::MemoryUtilities::align_to(single_tile_raw, 64);
    single_tile_padding = single_tile_stride - single_tile_raw;

    if (single_tile_padding > 0) {
        SubBlockInfo tail_pad;
        tail_pad.name = "64B Inter-Tile Pad";
        tail_pad.intra_offset = single_tile_raw;
        tail_pad.byte_size = single_tile_padding;
        tail_pad.is_padding = true;
        cached_sub_blocks.push_back(tail_pad);
    }

    // 2. Build Macro Tile Array
    int total_tiles = active_buffer->get_tile_count();
    for (int t = 0; t < total_tiles; ++t) {
        TileLayout tl;
        tl.tile_index = t;
        tl.element_start = t * elements_per_tile;
        tl.element_count = Math::min(elements_per_tile, max_elements - tl.element_start);
        tl.tile_offset_bytes = static_cast<size_t>(t) * single_tile_stride;
        tl.raw_bytes = single_tile_raw;
        tl.stride_bytes = single_tile_stride;
        tl.padding_bytes = single_tile_padding;
        cached_tiles.push_back(tl);
    }

    focused_tile_index = CLAMP(focused_tile_index, 0, Math::max(0, total_tiles - 1));
}

void TiledSoAVisualizer::_draw_crosshatch(const Rect2& p_rect, const Color& p_line_color, float p_spacing) {
    if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) return;

    float w = p_rect.size.x;
    float h = p_rect.size.y;

    for (float d = -h; d < w; d += p_spacing) {
        float t_min = Math::max(0.0f, -d);
        float t_max = Math::min(h, w - d);
        if (t_min < t_max) {
            Vector2 p1(p_rect.position.x + d + t_min, p_rect.position.y + t_min);
            Vector2 p2(p_rect.position.x + d + t_max, p_rect.position.y + t_max);
            draw_line(p1, p2, p_line_color, 1.0f);
        }
    }
}

void TiledSoAVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_tile_index != -1 || hovered_sub_block_index != -1) {
            hovered_tile_index = -1;
            hovered_sub_block_index = -1;
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 12;

        if (!active_buffer.is_valid() || cached_tiles.is_empty() || single_tile_stride <= 0) {
            Color placeholder_col = Color(0.5f, 0.5f, 0.55f, 0.8f);
            String msg = "No Tiled SoA schema configured. Set Elements Per Tile and add columns.";
            if (font.is_valid()) {
                Vector2 text_sz = font->get_string_size(msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                draw_string(font, (size - text_sz) / 2.0f + Vector2(0, text_sz.y), msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, placeholder_col);
            }
            return;
        }

        // Palette
        Color text_primary = has_theme_color("text_color", "TiledSoAVisualizer")
                           ? get_theme_color("text_color", "TiledSoAVisualizer")
                           : Color(0.95f, 0.95f, 0.95f, 1.0f);
        Color text_muted = Color(0.65f, 0.65f, 0.70f, 0.9f);
        Color border_col = Color(0.12f, 0.12f, 0.15f, 1.0f);
        Color pad_bg = Color(0.10f, 0.10f, 0.12f, 1.0f);
        Color pad_cross = Color(0.35f, 0.35f, 0.40f, 0.75f);
        Color tile_active_border = Color(0.95f, 0.85f, 0.35f, 1.0f);
        Color locality_tiled_col = Color(0.10f, 0.75f, 0.45f, 0.9f);
        Color locality_soa_col = Color(0.85f, 0.25f, 0.25f, 0.85f);

        int total_tiles = cached_tiles.size();
        int elements_per_tile = active_buffer->get_elements_per_tile();
        int max_elements = active_buffer->get_max_elements();

        // Vertical Section Allocations
        float header_h = 24.0f;
        float macro_h = 32.0f;
        float micro_header_h = 20.0f;
        float micro_h = 44.0f;
        float locality_h = show_locality_comparison ? 50.0f : 0.0f;
        float footer_h = 22.0f;

        float cur_y = 0.0f;

        // --- 1. Header Information ---
        if (font.is_valid()) {
            String header_str = String("TILED STRUCTURE OF ARRAYS: ") + String::num_int64(total_tiles) + " Tiles | " +
                                String::num_int64(elements_per_tile) + " Elems/Tile | Tile Stride: " +
                                String::num_int64(single_tile_stride) + " B (64B Cache-Aligned)";
            draw_string(font, Vector2(0.0f, header_h - 7.0f), header_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_primary);
        }
        cur_y += header_h;

        // --- 2. Macro Tile Track (Sequence of all tiles across buffer) ---
        float tile_spacing = 3.0f;
        float total_tile_spacing = tile_spacing * (total_tiles - 1);
        float macro_tile_w = (size.x - total_tile_spacing) / static_cast<float>(total_tiles);
        macro_tile_w = Math::max(macro_tile_w, 2.0f);

        for (int t = 0; t < total_tiles; ++t) {
            TileLayout& tl = cached_tiles.write[t];
            float tx = t * (macro_tile_w + tile_spacing);
            tl.macro_rect = Rect2(tx, cur_y, macro_tile_w, macro_h);

            // Alternating subtle tile tint
            Color tile_bg = (t % 2 == 0) ? Color(0.18f, 0.22f, 0.28f, 1.0f) : Color(0.24f, 0.28f, 0.35f, 1.0f);
            draw_rect(tl.macro_rect, tile_bg);

            // 64B Padding Waste segment on the right of each tile
            if (single_tile_stride > 0 && single_tile_padding > 0) {
                float pad_ratio = static_cast<float>(single_tile_padding) / static_cast<float>(single_tile_stride);
                float pw = macro_tile_w * pad_ratio;
                Rect2 pad_rect(tx + macro_tile_w - pw, cur_y, pw, macro_h);
                draw_rect(pad_rect, pad_bg);
                if (pw >= 4.0f) {
                    _draw_crosshatch(pad_rect, pad_cross, 4.0f);
                }
            }

            // Tile Border & Focused Highlight
            if (t == focused_tile_index) {
                draw_rect(tl.macro_rect, tile_active_border, false, 2.0f);
            } else {
                draw_rect(tl.macro_rect, border_col, false, 1.0f);
            }

            // Macro Tile Text (if space permits)
            if (font.is_valid() && macro_tile_w > 36.0f) {
                String t_lbl = "T" + String::num_int64(t);
                Vector2 str_sz = font->get_string_size(t_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2);
                Vector2 t_pos = tl.macro_rect.position + Vector2((macro_tile_w - str_sz.x) / 2.0f, (macro_h * 0.65f));
                draw_string(font, t_pos, t_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, text_primary);
            }
        }
        cur_y += macro_h + 8.0f;

        // --- 3. Micro Tile Track (Detailed SoA sub-block breakdown of focused tile) ---
        if (font.is_valid()) {
            String micro_hdr = String("FOCUSING TILE [") + String::num_int64(focused_tile_index) + "]: Elements " +
                               String::num_int64(cached_tiles[focused_tile_index].element_start) + ".." +
                               String::num_int64(cached_tiles[focused_tile_index].element_start + cached_tiles[focused_tile_index].element_count - 1) +
                               " | Offset: 0x" + String::num_uint64(cached_tiles[focused_tile_index].tile_offset_bytes, 16, true);
            draw_string(font, Vector2(0.0f, cur_y + micro_header_h - 6.0f), micro_hdr, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 1, text_primary);
        }
        cur_y += micro_header_h;

        float pixels_per_byte = size.x / static_cast<float>(single_tile_stride);
        float sub_x = 0.0f;

        for (int b = 0; b < cached_sub_blocks.size(); ++b) {
            SubBlockInfo& blk = cached_sub_blocks.write[b];
            float bw = blk.byte_size * pixels_per_byte;
            blk.rect = Rect2(sub_x, cur_y, bw, micro_h);

            if (blk.is_padding) {
                draw_rect(blk.rect, pad_bg);
                _draw_crosshatch(blk.rect, pad_cross, 6.0f);
            } else {
                Color c = _get_type_color(blk.data_type);
                draw_rect(blk.rect, c);
            }

            draw_rect(blk.rect, border_col, false, 1.5f);

            // Sub-block labels
            if (font.is_valid() && bw > 14.0f) {
                String top_lbl = blk.is_padding ? "PAD" : blk.name;
                String btm_lbl = String::num_int64(blk.byte_size) + "B";

                Vector2 top_sz = font->get_string_size(top_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2);
                Vector2 btm_sz = font->get_string_size(btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 3);

                if (top_sz.x < (bw - 4.0f)) {
                    Vector2 top_pos = blk.rect.position + Vector2((bw - top_sz.x) / 2.0f, micro_h * 0.45f);
                    draw_string(font, top_pos + Vector2(1, 1), top_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, Color(0, 0, 0, 0.9f));
                    draw_string(font, top_pos, top_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, text_primary);

                    if (btm_sz.x < (bw - 4.0f)) {
                        Vector2 btm_pos = blk.rect.position + Vector2((bw - btm_sz.x) / 2.0f, micro_h * 0.85f);
                        draw_string(font, btm_pos + Vector2(1, 1), btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 3, Color(0, 0, 0, 0.9f));
                        draw_string(font, btm_pos, btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 3, text_muted);
                    }
                } else if (btm_sz.x < (bw - 2.0f)) {
                    Vector2 btm_pos = blk.rect.position + Vector2((bw - btm_sz.x) / 2.0f, micro_h * 0.65f);
                    draw_string(font, btm_pos, btm_lbl, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 3, text_primary);
                }
            }

            sub_x += bw;
        }

        // Sub-block hover outline
        if (hovered_sub_block_index >= 0 && hovered_sub_block_index < cached_sub_blocks.size()) {
            draw_rect(cached_sub_blocks[hovered_sub_block_index].rect, Color(1.0f, 1.0f, 1.0f, 0.9f), false, 2.0f);
        }
        cur_y += micro_h + 10.0f;

        // --- 4. Spatial Cache Locality Comparison Diagram ---
        if (show_locality_comparison && font.is_valid()) {
            size_t total_soa_span = 0;
            TypedArray<Dictionary> cols = active_buffer->get_columns();
            for (int i = 0; i < cols.size(); ++i) {
                Dictionary col = cols[i];
                int s = col.has("size") ? static_cast<int>(col["size"]) : 4;
                total_soa_span += static_cast<size_t>(max_elements) * static_cast<size_t>(s);
            }

            double gain = (single_tile_stride > 0) ? (static_cast<double>(total_soa_span) / static_cast<double>(single_tile_stride)) : 1.0;

            String locality_title = String("CACHE LOCALITY COMPARISON (Entity Working-Set Span):");
            draw_string(font, Vector2(0.0f, cur_y + 12.0f), locality_title, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
            cur_y += 16.0f;

            float bar_w = size.x;
            float bar_h = 10.0f;

            // Tiled SoA working set footprint
            Rect2 tiled_loc_rect(0.0f, cur_y, bar_w * Math::min(1.0, 0.25 + (0.15 * Math::min(4.0, gain))), bar_h);
            draw_rect(tiled_loc_rect, locality_tiled_col);
            draw_rect(tiled_loc_rect, border_col, false, 1.0f);

            String t_stat = "Tiled SoA: " + String::num_int64(single_tile_stride) + " B working set (L1/L2 resident) -> " +
                            String::num_real(gain, false) + "x tighter locality than pure SoA";
            draw_string(font, Vector2(0.0f, cur_y + bar_h + 12.0f), t_stat, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, locality_tiled_col);

            cur_y += bar_h + 16.0f;
        }

        // --- 5. Footer Diagnostics ---
        if (font.is_valid()) {
            int total_footprint = active_buffer->calculate_raw_data_bytes();
            String f_str = "Total Tiled Allocation: " + String::num_uint64(total_footprint) + " Bytes (" +
                           String::num_real(static_cast<double>(total_footprint) / 1024.0, false) + " KB) | Click a macro tile to focus.";
            draw_string(font, Vector2(0.0f, size.y - 6.0f), f_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
        }
    }
}

void TiledSoAVisualizer::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        Vector2 pos = mm->get_position();
        int new_hovered_tile = -1;
        int new_hovered_sub = -1;

        for (int t = 0; t < cached_tiles.size(); ++t) {
            if (cached_tiles[t].macro_rect.has_point(pos)) {
                new_hovered_tile = t;
                break;
            }
        }

        for (int b = 0; b < cached_sub_blocks.size(); ++b) {
            if (cached_sub_blocks[b].rect.has_point(pos)) {
                new_hovered_sub = b;
                break;
            }
        }

        if (new_hovered_tile != hovered_tile_index || new_hovered_sub != hovered_sub_block_index) {
            hovered_tile_index = new_hovered_tile;
            hovered_sub_block_index = new_hovered_sub;
            queue_redraw();
        }
    }

    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
        Vector2 pos = mb->get_position();
        for (int t = 0; t < cached_tiles.size(); ++t) {
            if (cached_tiles[t].macro_rect.has_point(pos)) {
                set_focused_tile_index(t);
                accept_event();
                return;
            }
        }
    }

    Control::_gui_input(p_event);
}

String TiledSoAVisualizer::_get_tooltip(const Vector2& p_at_position) const {
    if (!active_buffer.is_valid()) return String();

    // Check Micro Sub-Blocks
    for (int b = 0; b < cached_sub_blocks.size(); ++b) {
        const SubBlockInfo& blk = cached_sub_blocks[b];
        if (blk.rect.has_point(p_at_position)) {
            if (blk.is_padding) {
                return String("[Inter-Tile Alignment Padding]\n") +
                       "Padding Size: " + String::num_int64(blk.byte_size) + " Bytes\n" +
                       "Rule: Aligns next tile start to 64-byte hardware cache line boundary";
            } else {
                int elems_per_tile = active_buffer->get_elements_per_tile();
                return String("[Tile Sub-Stream: ") + blk.name + "]\n" +
                       "Type: " + _get_type_name(blk.data_type) + " " + _get_type_badge(blk.data_type) + "\n" +
                       "Elements in Tile: " + String::num_int64(elems_per_tile) + "\n" +
                       "Sub-Stream Size: " + String::num_int64(blk.byte_size) + " Bytes (" +
                       String::num_int64(blk.col_size) + " B/elem)\n" +
                       "Intra-Tile Offset: 0x" + String::num_uint64(blk.intra_offset, 16, true) + " (" +
                       String::num_uint64(blk.intra_offset) + " B)";
            }
        }
    }

    // Check Macro Tiles
    for (int t = 0; t < cached_tiles.size(); ++t) {
        const TileLayout& tl = cached_tiles[t];
        if (tl.macro_rect.has_point(p_at_position)) {
            return String("[Tile #") + String::num_int64(tl.tile_index) + "]\n" +
                   "Element Range: [" + String::num_int64(tl.element_start) + " .. " +
                   String::num_int64(tl.element_start + tl.element_count - 1) + "]\n" +
                   "Tile Offset: 0x" + String::num_uint64(tl.tile_offset_bytes, 16, true) + " (" +
                   String::num_uint64(tl.tile_offset_bytes) + " B)\n" +
                   "Tile Raw Size: " + String::num_int64(tl.raw_bytes) + " Bytes\n" +
                   "Tile 64B Stride: " + String::num_int64(tl.stride_bytes) + " Bytes\n" +
                   "Alignment Slack: " + String::num_int64(tl.padding_bytes) + " Bytes\n" +
                   "(Click to focus internal sub-blocks)";
        }
    }

    return Control::_get_tooltip(p_at_position);
}

void TiledSoAVisualizer::set_focused_tile_index(int p_index) {
    if (focused_tile_index == p_index) return;
    int max_tile = MAX(0, cached_tiles.size() - 1);
    focused_tile_index = CLAMP(p_index, 0, max_tile);
    queue_redraw();
}
int TiledSoAVisualizer::get_focused_tile_index() const { return focused_tile_index; }

void TiledSoAVisualizer::set_show_locality_comparison(bool p_show) {
    if (show_locality_comparison == p_show) return;
    show_locality_comparison = p_show;
    queue_redraw();
    update_minimum_size();
}
bool TiledSoAVisualizer::get_show_locality_comparison() const { return show_locality_comparison; }

Vector2 TiledSoAVisualizer::_get_minimum_size() const {
    float h = 130.0f;
    if (show_locality_comparison) h += 46.0f;
    return Vector2(160.0f, h);
}

} // namespace ideam::godot_ext