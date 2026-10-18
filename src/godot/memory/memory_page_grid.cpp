#include "memory_page_grid.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryPageGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &MemoryPageGrid::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &MemoryPageGrid::_on_buffer_changed);

    ClassDB::bind_method(D_METHOD("set_page_size", "size"), &MemoryPageGrid::set_page_size);
    ClassDB::bind_method(D_METHOD("get_page_size"), &MemoryPageGrid::get_page_size);

    ClassDB::bind_method(D_METHOD("set_target_columns", "columns"), &MemoryPageGrid::set_target_columns);
    ClassDB::bind_method(D_METHOD("get_target_columns"), &MemoryPageGrid::get_target_columns);

    ClassDB::bind_method(D_METHOD("set_enforce_square_cells", "enforce"), &MemoryPageGrid::set_enforce_square_cells);
    ClassDB::bind_method(D_METHOD("get_enforce_square_cells"), &MemoryPageGrid::get_enforce_square_cells);

    ClassDB::bind_method(D_METHOD("set_access_strategy", "strategy"), &MemoryPageGrid::set_access_strategy);
    ClassDB::bind_method(D_METHOD("get_access_strategy"), &MemoryPageGrid::get_access_strategy);

    ClassDB::bind_method(D_METHOD("set_virtual_headroom_pages", "count"), &MemoryPageGrid::set_virtual_headroom_pages);
    ClassDB::bind_method(D_METHOD("get_virtual_headroom_pages"), &MemoryPageGrid::get_virtual_headroom_pages);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "page_size"), "set_page_size", "get_page_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "target_columns", PROPERTY_HINT_RANGE, "1,128,1"), "set_target_columns", "get_target_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enforce_square_cells"), "set_enforce_square_cells", "get_enforce_square_cells");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "access_strategy", PROPERTY_HINT_ENUM, "Linear Scan:0,Random Scatter:1"), "set_access_strategy", "get_access_strategy");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "virtual_headroom_pages"), "set_virtual_headroom_pages", "get_virtual_headroom_pages");
}

MemoryPageGrid::MemoryPageGrid() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
}

MemoryPageGrid::~MemoryPageGrid() {
    _clear_connections();
}

void MemoryPageGrid::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void MemoryPageGrid::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
        page_size = active_buffer->get_page_size_bytes();
    }

    hovered_layer = -1;
    hovered_vpn = -1;
    hovered_pfn = -1;

    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

void MemoryPageGrid::_on_buffer_changed() {
    if (active_buffer.is_valid()) {
        page_size = active_buffer->get_page_size_bytes();
    }
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}

void MemoryPageGrid::_recalculate_layout() {
    virtual_pages.clear();
    physical_frames.clear();

    if (!active_buffer.is_valid() || page_size <= 0) return;

    size_t raw_bytes = static_cast<size_t>(active_buffer->calculate_raw_data_bytes());
    int committed_pages = active_buffer->get_page_count();
    if (committed_pages <= 0 && raw_bytes > 0) {
        committed_pages = static_cast<int>((raw_bytes + page_size - 1) / page_size);
    }
    if (committed_pages == 0) committed_pages = 1;

    int total_virtual = committed_pages + virtual_headroom_pages;

    // 1. Build Virtual Page Directory (VPN)
    size_t remaining_data = raw_bytes;
    for (int v = 0; v < total_virtual; ++v) {
        VirtualPageEntry vpe;
        vpe.vpn = v;

        if (v < committed_pages) {
            // Simulated mapping: Scatter or sequential frame distribution
            vpe.pfn = (access_strategy == ACCESS_RANDOM_SCATTER)
                    ? ((v * 7 + 3) % committed_pages)
                    : v;

            size_t bytes_in_page = Math::min(remaining_data, static_cast<size_t>(page_size));
            vpe.used_bytes = bytes_in_page;
            vpe.slack_bytes = static_cast<size_t>(page_size) - bytes_in_page;
            vpe.state = (bytes_in_page == static_cast<size_t>(page_size))
                      ? PAGE_STATE_COMMITTED_FULL
                      : PAGE_STATE_COMMITTED_PARTIAL;
            remaining_data = (remaining_data > bytes_in_page) ? (remaining_data - bytes_in_page) : 0;
        } else {
            vpe.pfn = -1;
            vpe.state = PAGE_STATE_UNCOMMITTED;
            vpe.used_bytes = 0;
            vpe.slack_bytes = page_size;
        }

        virtual_pages.push_back(vpe);
    }

    // 2. Build Physical Page Frames (PFN)
    for (int p = 0; p < committed_pages; ++p) {
        PhysicalFrameEntry pfe;
        pfe.pfn = p;
        pfe.vpn = -1;

        // Find which VPN mapped to this frame
        for (int v = 0; v < virtual_pages.size(); ++v) {
            if (virtual_pages[v].pfn == p) {
                pfe.vpn = virtual_pages[v].vpn;
                pfe.state = virtual_pages[v].state;
                pfe.used_bytes = virtual_pages[v].used_bytes;
                pfe.slack_bytes = virtual_pages[v].slack_bytes;
                break;
            }
        }

        physical_frames.push_back(pfe);
    }
}

void MemoryPageGrid::_draw_crosshatch(const Rect2& p_rect, const Color& p_line_color, float p_spacing) {
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

void MemoryPageGrid::_draw_bezier_link(const Vector2& p_start, const Vector2& p_end, const Color& p_color, float p_width) {
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

void MemoryPageGrid::_draw_access_strategy_overlay(const Vector2& p_size) {
    if (virtual_pages.is_empty() || physical_frames.is_empty()) return;

    Color seq_arrow_col = Color(0.2f, 0.85f, 0.55f, 0.6f);
    Color scatter_pulse_col = Color(0.95f, 0.45f, 0.15f, 0.65f);

    if (access_strategy == ACCESS_LINEAR_SCAN) {
        // Draw sequential directional connectors across the directory layer
        for (int i = 0; i < virtual_pages.size() - 1; ++i) {
            if (virtual_pages[i].state == PAGE_STATE_UNCOMMITTED) break;
            Vector2 pt_a = virtual_pages[i].rect.position + (virtual_pages[i].rect.size * 0.5f);
            Vector2 pt_b = virtual_pages[i + 1].rect.position + (virtual_pages[i + 1].rect.size * 0.5f);
            draw_line(pt_a, pt_b, seq_arrow_col, 2.0f);

            // Subtle arrowhead indicator
            Vector2 dir = (pt_b - pt_a).normalized();
            Vector2 perp(-dir.y, dir.x);
            Vector2 mid = (pt_a + pt_b) * 0.5f;
            draw_line(mid, mid - (dir * 4.0f) + (perp * 3.0f), seq_arrow_col, 1.5f);
            draw_line(mid, mid - (dir * 4.0f) - (perp * 3.0f), seq_arrow_col, 1.5f);
        }
    } else {
        // Draw non-contiguous hopping lines across the physical frame pool
        for (int i = 0; i < physical_frames.size() - 1; ++i) {
            Vector2 pt_a = physical_frames[i].rect.position + Vector2(physical_frames[i].rect.size.x * 0.5f, 0.0f);
            int next_p = (i * 3 + 1) % physical_frames.size();
            Vector2 pt_b = physical_frames[next_p].rect.position + Vector2(physical_frames[next_p].rect.size.x * 0.5f, 0.0f);

            Vector2 cp( (pt_a.x + pt_b.x) * 0.5f, routing_channel_y + (routing_channel_h * 0.7f) );
            draw_line(pt_a, cp, scatter_pulse_col, 1.0f);
            draw_line(cp, pt_b, scatter_pulse_col, 1.0f);
        }
    }
}

void MemoryPageGrid::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_layer != -1 || hovered_vpn != -1 || hovered_pfn != -1) {
            hovered_layer = -1;
            hovered_vpn = -1;
            hovered_pfn = -1;
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        Vector2 size = get_size();
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 12;

        if (!active_buffer.is_valid() || virtual_pages.is_empty()) {
            Color placeholder_col = Color(0.5f, 0.5f, 0.55f, 0.8f);
            String msg = "No Paged Buffer active. Define columns and allocate capacity.";
            if (font.is_valid()) {
                Vector2 text_sz = font->get_string_size(msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                draw_string(font, (size - text_sz) / 2.0f + Vector2(0, text_sz.y), msg, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, placeholder_col);
            }
            return;
        }

        // Palette
        Color text_primary = Color(0.95f, 0.95f, 0.95f, 1.0f);
        Color text_muted = Color(0.65f, 0.65f, 0.70f, 0.9f);
        Color border_col = Color(0.12f, 0.12f, 0.15f, 1.0f);
        Color dir_bg = Color(0.13f, 0.14f, 0.18f, 1.0f);
        Color phys_bg = Color(0.10f, 0.10f, 0.13f, 1.0f);

        Color full_page_col = Color(0.12f, 0.58f, 0.85f, 0.95f);    // Solid Ocean Blue
        Color partial_page_col = Color(0.06f, 0.72f, 0.51f, 0.95f); // Emerald
        Color slack_col = Color(0.85f, 0.35f, 0.25f, 0.85f);        // Coral Red (Slack Waste)
        Color slack_cross_col = Color(0.35f, 0.10f, 0.08f, 0.90f);
        Color uncommitted_border = Color(0.45f, 0.45f, 0.50f, 0.5f);
        Color trace_col = Color(0.96f, 0.78f, 0.18f, 1.0f);          // Gold Highlight

        float header_h = 24.0f;
        float dir_label_h = 18.0f;
        float dir_h = 36.0f;
        float routing_h = 42.0f;
        float phys_label_h = 18.0f;
        float phys_h = 48.0f;
        float footer_h = 22.0f;

        float cur_y = 0.0f;

        // --- 1. Top Telemetry Header ---
        if (font.is_valid()) {
            size_t raw_bytes = static_cast<size_t>(active_buffer->calculate_raw_data_bytes());
            int committed_count = physical_frames.size();
            size_t total_page_bytes = static_cast<size_t>(committed_count) * static_cast<size_t>(page_size);
            size_t total_slack = (total_page_bytes >= raw_bytes) ? (total_page_bytes - raw_bytes) : 0;
            double slack_pct = (total_page_bytes > 0) ? (static_cast<double>(total_slack) / static_cast<double>(total_page_bytes) * 100.0) : 0.0;

            String hdr = String("PAGE TABLE ARCHITECTURE: Page Size = ") + String::num_int64(page_size) + " B (" +
                         String::num_real(static_cast<double>(page_size) / 1024.0, false) + " KB) | " +
                         String::num_int64(committed_count) + " Committed Pages | Slack: " +
                         String::num_uint64(total_slack) + " B (" + String::num_real(slack_pct, false) + "%)";
            draw_string(font, Vector2(0.0f, header_h - 7.0f), hdr, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_primary);

            String strat_lbl = (access_strategy == ACCESS_LINEAR_SCAN) ? "[ACCESS: LINEAR SCAN]" : "[ACCESS: RANDOM SCATTER]";
            Vector2 strat_sz = font->get_string_size(strat_lbl, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size);
            draw_string(font, Vector2(size.x - strat_sz.x, header_h - 7.0f), strat_lbl, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size,
                        (access_strategy == ACCESS_LINEAR_SCAN) ? Color(0.2f, 0.85f, 0.55f) : Color(0.95f, 0.55f, 0.2f));
        }
        cur_y += header_h;

        // --- 2. Page Directory Table (Upper Layer) ---
        if (font.is_valid()) {
            draw_string(font, Vector2(0.0f, cur_y + dir_label_h - 4.0f), "PAGE DIRECTORY (Virtual Address Space / VPNs):", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
        }
        cur_y += dir_label_h;

        directory_tier_rect = Rect2(0.0f, cur_y, size.x, dir_h);
        draw_rect(directory_tier_rect, dir_bg);

        int total_virt_pages = virtual_pages.size();
        float cell_spacing = 3.0f;
        float virt_cell_w = (size.x - (cell_spacing * (total_virt_pages - 1))) / static_cast<float>(total_virt_pages);
        virt_cell_w = Math::max(virt_cell_w, 4.0f);

        for (int v = 0; v < total_virt_pages; ++v) {
            VirtualPageEntry& vpe = virtual_pages.write[v];
            float vx = v * (virt_cell_w + cell_spacing);
            vpe.rect = Rect2(vx, cur_y, virt_cell_w, dir_h);

            if (vpe.state == PAGE_STATE_COMMITTED_FULL) {
                draw_rect(vpe.rect, full_page_col);
            } else if (vpe.state == PAGE_STATE_COMMITTED_PARTIAL) {
                draw_rect(vpe.rect, partial_page_col);
            } else {
                draw_rect(vpe.rect, Color(0.16f, 0.16f, 0.20f, 0.7f));
                _draw_crosshatch(vpe.rect, Color(0.25f, 0.25f, 0.30f, 0.5f), 8.0f);
            }

            draw_rect(vpe.rect, (vpe.state == PAGE_STATE_UNCOMMITTED) ? uncommitted_border : border_col, false, 1.0f);

            if (font.is_valid() && virt_cell_w > 34.0f) {
                String v_lbl = "V" + String::num_int64(vpe.vpn);
                String p_sub = (vpe.pfn >= 0) ? ("P" + String::num_int64(vpe.pfn)) : "-";
                draw_string(font, vpe.rect.position + Vector2(4.0f, dir_h * 0.45f), v_lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 3, text_primary);
                draw_string(font, vpe.rect.position + Vector2(4.0f, dir_h * 0.85f), p_sub, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 3, text_muted);
            }
        }
        cur_y += dir_h;

        // --- 3. Mid-Channel Routing & Strategy Visualization ---
        routing_channel_y = cur_y;
        routing_channel_h = routing_h;
        _draw_access_strategy_overlay(size);
        cur_y += routing_h;

        // --- 4. Physical Frame Allocation Pool (Lower Layer) ---
        if (font.is_valid()) {
            draw_string(font, Vector2(0.0f, cur_y + phys_label_h - 4.0f), "PHYSICAL FRAME ALLOCATIONS (Committed Backing Frames):", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
        }
        cur_y += phys_label_h;

        physical_tier_rect = Rect2(0.0f, cur_y, size.x, phys_h);
        draw_rect(physical_tier_rect, phys_bg);

        int total_phys_frames = physical_frames.size();
        float phys_cell_w = (size.x - (cell_spacing * (total_phys_frames - 1))) / static_cast<float>(total_phys_frames);
        phys_cell_w = Math::max(phys_cell_w, 4.0f);

        for (int p = 0; p < total_phys_frames; ++p) {
            PhysicalFrameEntry& pfe = physical_frames.write[p];
            float px = p * (phys_cell_w + cell_spacing);
            pfe.rect = Rect2(px, cur_y, phys_cell_w, phys_h);

            float fill_ratio = static_cast<float>(pfe.used_bytes) / static_cast<float>(page_size);
            float used_w = phys_cell_w * fill_ratio;

            // Draw used payload area
            if (used_w > 0.0f) {
                draw_rect(Rect2(px, cur_y, used_w, phys_h), (pfe.state == PAGE_STATE_COMMITTED_FULL) ? full_page_col : partial_page_col);
            }

            // Draw internal fragmentation slack area
            if (pfe.slack_bytes > 0 && (phys_cell_w - used_w) > 0.0f) {
                Rect2 slack_rect(px + used_w, cur_y, phys_cell_w - used_w, phys_h);
                draw_rect(slack_rect, slack_col);
                if (slack_rect.size.x >= 6.0f) {
                    _draw_crosshatch(slack_rect, slack_cross_col, 5.0f);
                }
            }

            draw_rect(pfe.rect, border_col, false, 1.5f);

            if (font.is_valid() && phys_cell_w > 40.0f) {
                String f_lbl = "Frame #" + String::num_int64(pfe.pfn);
                String f_stat = String::num_int64(pfe.used_bytes) + "B";
                draw_string(font, pfe.rect.position + Vector2(4.0f, phys_h * 0.40f), f_lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_primary);
                draw_string(font, pfe.rect.position + Vector2(4.0f, phys_h * 0.80f), f_stat, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 3, text_primary);
            }
        }
        cur_y += phys_h;

        // --- 5. Interactive Trace Mapping ---
        int target_vpn = -1;
        int target_pfn = -1;

        if (hovered_layer == 0 && hovered_vpn >= 0 && hovered_vpn < virtual_pages.size()) {
            target_vpn = hovered_vpn;
            target_pfn = virtual_pages[target_vpn].pfn;
        } else if (hovered_layer == 1 && hovered_pfn >= 0 && hovered_pfn < physical_frames.size()) {
            target_pfn = hovered_pfn;
            target_vpn = physical_frames[target_pfn].vpn;
        }

        if (target_vpn >= 0 && target_vpn < virtual_pages.size()) {
            const VirtualPageEntry& v_entry = virtual_pages[target_vpn];
            draw_rect(v_entry.rect, trace_col, false, 2.0f);

            if (target_pfn >= 0 && target_pfn < physical_frames.size()) {
                const PhysicalFrameEntry& p_entry = physical_frames[target_pfn];
                draw_rect(p_entry.rect, trace_col, false, 2.0f);

                Vector2 start_pt(v_entry.rect.position.x + (v_entry.rect.size.x * 0.5f), v_entry.rect.position.y + v_entry.rect.size.y);
                Vector2 end_pt(p_entry.rect.position.x + (p_entry.rect.size.x * 0.5f), p_entry.rect.position.y);
                _draw_bezier_link(start_pt, end_pt, trace_col, 2.5f);

                if (font.is_valid()) {
                    Vector2 mid = (start_pt + end_pt) * 0.5f;
                    String badge = "VPN " + String::num_int64(target_vpn) + " ➔ PFN " + String::num_int64(target_pfn);
                    Vector2 b_sz = font->get_string_size(badge, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2);
                    Rect2 b_rect(mid.x - (b_sz.x * 0.5f) - 6.0f, mid.y - 9.0f, b_sz.x + 12.0f, 18.0f);
                    draw_rect(b_rect, Color(0.10f, 0.10f, 0.14f, 0.92f));
                    draw_rect(b_rect, trace_col, false, 1.0f);
                    draw_string(font, Vector2(b_rect.position.x + 6.0f, b_rect.position.y + 13.0f), badge, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size - 2, text_primary);
                }
            } else {
                // Ghost uncommitted link
                Vector2 drop_pt(v_entry.rect.position.x + (v_entry.rect.size.x * 0.5f), v_entry.rect.position.y + v_entry.rect.size.y + 16.0f);
                draw_line(Vector2(drop_pt.x, v_entry.rect.position.y + v_entry.rect.size.y), drop_pt, Color(0.7f, 0.7f, 0.75f, 0.8f), 1.5f);
            }
        }

        // --- 6. Footer Telemetry ---
        if (font.is_valid()) {
            int footprint = active_buffer->calculate_projected_footprint_bytes();
            String f_str = "Committed Physical Footprint: " + String::num_uint64(footprint) + " Bytes (" +
                           String::num_real(static_cast<double>(footprint) / 1024.0, false) + " KB) | Hover page or frame to inspect address translation.";
            draw_string(font, Vector2(0.0f, size.y - 6.0f), f_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 2, text_muted);
        }
    }
}

void MemoryPageGrid::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        Vector2 pos = mm->get_position();

        int new_hovered_layer = -1;
        int new_hovered_vpn = -1;
        int new_hovered_pfn = -1;

        if (directory_tier_rect.has_point(pos)) {
            new_hovered_layer = 0;
            for (int v = 0; v < virtual_pages.size(); ++v) {
                if (virtual_pages[v].rect.has_point(pos)) {
                    new_hovered_vpn = v;
                    break;
                }
            }
        } else if (physical_tier_rect.has_point(pos)) {
            new_hovered_layer = 1;
            for (int p = 0; p < physical_frames.size(); ++p) {
                if (physical_frames[p].rect.has_point(pos)) {
                    new_hovered_pfn = p;
                    break;
                }
            }
        }

        if (new_hovered_layer != hovered_layer || new_hovered_vpn != hovered_vpn || new_hovered_pfn != hovered_pfn) {
            hovered_layer = new_hovered_layer;
            hovered_vpn = new_hovered_vpn;
            hovered_pfn = new_hovered_pfn;
            queue_redraw();
        }
    }

    Control::_gui_input(p_event);
}

String MemoryPageGrid::_get_tooltip(const Vector2& p_at_position) const {
    if (!active_buffer.is_valid()) return String();

    if (directory_tier_rect.has_point(p_at_position)) {
        for (int v = 0; v < virtual_pages.size(); ++v) {
            const VirtualPageEntry& vpe = virtual_pages[v];
            if (vpe.rect.has_point(p_at_position)) {
                size_t v_start = static_cast<size_t>(vpe.vpn) * static_cast<size_t>(page_size);
                size_t v_end = v_start + page_size - 1;

                String state_str = (vpe.state == PAGE_STATE_COMMITTED_FULL) ? "Committed (Full)"
                                 : ((vpe.state == PAGE_STATE_COMMITTED_PARTIAL) ? "Committed (Partially Filled)" : "Uncommitted (Virtual Reserve)");

                return String("[Virtual Page #") + String::num_int64(vpe.vpn) + " (VPN)]\n" +
                       "State: " + state_str + "\n" +
                       "Virtual Address Range: 0x" + String::num_uint64(v_start, 16, true) + " .. 0x" + String::num_uint64(v_end, 16, true) + "\n" +
                       "Mapped Physical Frame: " + ((vpe.pfn >= 0) ? ("PFN #" + String::num_int64(vpe.pfn)) : "None (Page Fault / Ghost)") + "\n" +
                       "Payload: " + String::num_uint64(vpe.used_bytes) + " B | Internal Slack: " + String::num_uint64(vpe.slack_bytes) + " B";
            }
        }
    } else if (physical_tier_rect.has_point(p_at_position)) {
        for (int p = 0; p < physical_frames.size(); ++p) {
            const PhysicalFrameEntry& pfe = physical_frames[p];
            if (pfe.rect.has_point(p_at_position)) {
                size_t p_offset = static_cast<size_t>(pfe.pfn) * static_cast<size_t>(page_size);
                double slack_pct = (page_size > 0) ? (static_cast<double>(pfe.slack_bytes) / static_cast<double>(page_size) * 100.0) : 0.0;

                return String("[Physical Frame #") + String::num_int64(pfe.pfn) + " (PFN)]\n" +
                       "Backing Virtual Page: VPN #" + String::num_int64(pfe.vpn) + "\n" +
                       "Physical Base Offset: 0x" + String::num_uint64(p_offset, 16, true) + "\n" +
                       "Committed Size: " + String::num_int64(page_size) + " Bytes\n" +
                       "Active Data: " + String::num_uint64(pfe.used_bytes) + " Bytes\n" +
                       "Internal Fragmentation Slack: " + String::num_uint64(pfe.slack_bytes) + " Bytes (" + String::num_real(slack_pct, false) + "% waste)";
            }
        }
    }

    return Control::_get_tooltip(p_at_position);
}

Vector2 MemoryPageGrid::_get_minimum_size() const {
    return Vector2(160.0f, 186.0f);
}

// Property Setters
void MemoryPageGrid::set_page_size(int p_size) {
    int val = Math::max(64, p_size);
    if (page_size == val) return;
    page_size = val;
    if (active_buffer.is_valid()) {
        active_buffer->set_page_size_bytes(val);
    }
    _recalculate_layout();
    queue_redraw();
    update_minimum_size();
}
int MemoryPageGrid::get_page_size() const { return page_size; }

void MemoryPageGrid::set_target_columns(int p_columns) {
    if (target_columns == p_columns || p_columns <= 0) return;
    target_columns = p_columns;
    queue_redraw();
}
int MemoryPageGrid::get_target_columns() const { return target_columns; }

void MemoryPageGrid::set_enforce_square_cells(bool p_enforce) {
    if (enforce_square_cells == p_enforce) return;
    enforce_square_cells = p_enforce;
    queue_redraw();
}
bool MemoryPageGrid::get_enforce_square_cells() const { return enforce_square_cells; }

void MemoryPageGrid::set_access_strategy(int p_strategy) {
    AccessStrategy strat = static_cast<AccessStrategy>(p_strategy);
    if (access_strategy == strat) return;
    access_strategy = strat;
    _recalculate_layout();
    queue_redraw();
}
int MemoryPageGrid::get_access_strategy() const { return static_cast<int>(access_strategy); }

void MemoryPageGrid::set_virtual_headroom_pages(int p_count) {
    int val = Math::max(0, p_count);
    if (virtual_headroom_pages == val) return;
    virtual_headroom_pages = val;
    _recalculate_layout();
    queue_redraw();
}
int MemoryPageGrid::get_virtual_headroom_pages() const { return virtual_headroom_pages; }

} // namespace ideam::godot_ext