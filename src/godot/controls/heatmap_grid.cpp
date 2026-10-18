#include "heatmap_grid.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace ideam::godot_ext {

void HeatmapGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_data", "data"), &HeatmapGrid::set_data);
    ClassDB::bind_method(D_METHOD("get_data"), &HeatmapGrid::get_data);

    ClassDB::bind_method(D_METHOD("set_gradient", "gradient"), &HeatmapGrid::set_gradient);
    ClassDB::bind_method(D_METHOD("get_gradient"), &HeatmapGrid::get_gradient);

    ClassDB::bind_method(D_METHOD("set_columns", "columns"), &HeatmapGrid::set_columns);
    ClassDB::bind_method(D_METHOD("get_columns"), &HeatmapGrid::get_columns);

    ClassDB::bind_method(D_METHOD("set_square_cells", "square_cells"), &HeatmapGrid::set_square_cells);
    ClassDB::bind_method(D_METHOD("get_square_cells"), &HeatmapGrid::get_square_cells);
    
    ClassDB::bind_method(D_METHOD("set_value", "index", "value"), &HeatmapGrid::set_value);
    ClassDB::bind_method(D_METHOD("get_value", "index"), &HeatmapGrid::get_value);
    ClassDB::bind_method(D_METHOD("clear_data"), &HeatmapGrid::clear_data);
    ClassDB::bind_method(D_METHOD("resize_data", "size"), &HeatmapGrid::resize_data);

    ClassDB::bind_method(D_METHOD("set_selected_index", "index"), &HeatmapGrid::set_selected_index);
    ClassDB::bind_method(D_METHOD("get_selected_index"), &HeatmapGrid::get_selected_index);
    ClassDB::bind_method(D_METHOD("set_hovered_index", "index"), &HeatmapGrid::set_hovered_index);
    ClassDB::bind_method(D_METHOD("get_hovered_index"), &HeatmapGrid::get_hovered_index);

    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "color_gradient", PROPERTY_HINT_RESOURCE_TYPE, "Gradient"), "set_gradient", "get_gradient");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,1024,1"), "set_columns", "get_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "square_cells"), "set_square_cells", "get_square_cells");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "selected_index"), "set_selected_index", "get_selected_index");

    ADD_SIGNAL(MethodInfo("cell_hovered", PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("cell_clicked", PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::INT, "button_index")));
}

HeatmapGrid::HeatmapGrid() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_STOP);
}

Color HeatmapGrid::_get_color_for_value(uint8_t p_value) const {
    if (color_gradient.is_valid()) {
        float offset = static_cast<float>(p_value) / 255.0f;
        return color_gradient->sample(offset);
    }

    String color_name = "state_" + String::num_int64(p_value) + "_color";
    if (has_theme_color(color_name, "HeatmapGrid")) {
        return get_theme_color(color_name, "HeatmapGrid");
    }

    switch (p_value) {
        case STATE_UNSELECTED:
            return has_theme_color("unselected_color", "HeatmapGrid") 
                ? get_theme_color("unselected_color", "HeatmapGrid") 
                : Color(0.15f, 0.16f, 0.20f, 0.85f);
        case STATE_SELECTED:
            return has_theme_color("selected_color", "HeatmapGrid") 
                ? get_theme_color("selected_color", "HeatmapGrid") 
                : Color(0.12f, 0.75f, 0.90f, 1.0f); // Electric Cyan
        case STATE_HOVERED:
            return has_theme_color("hovered_color", "HeatmapGrid") 
                ? get_theme_color("hovered_color", "HeatmapGrid") 
                : Color(0.98f, 0.78f, 0.15f, 1.0f); // Amber / Gold
        case STATE_PENDING:
            return has_theme_color("pending_color", "HeatmapGrid") 
                ? get_theme_color("pending_color", "HeatmapGrid") 
                : Color(0.72f, 0.35f, 0.95f, 1.0f); // Violet
        default: {
            float intensity = static_cast<float>(p_value) / 255.0f;
            return Color(intensity, intensity * 0.5f, 0.2f, 1.0f);
        }
    }
}

int HeatmapGrid::_get_cell_index_at_pos(const Vector2& p_pos) const {
    if (data.is_empty() || columns <= 0) return -1;

    float margin = has_theme_constant("cell_margin", "HeatmapGrid") 
                 ? static_cast<float>(get_theme_constant("cell_margin", "HeatmapGrid")) 
                 : 1.5f;

    Vector2 size = get_size();
    int total_cells = data.size();
    int rows = (total_cells + columns - 1) / columns;

    float available_width = size.x - (margin * (columns + 1));
    float available_height = size.y - (margin * (rows + 1));
    available_width = MAX(available_width, 1.0f);
    available_height = MAX(available_height, 1.0f);

    float cell_w = available_width / static_cast<float>(columns);
    float cell_h = available_height / static_cast<float>(rows);

    if (square_cells) {
        float min_dim = MIN(cell_w, cell_h);
        cell_w = min_dim;
        cell_h = min_dim;
    }

    float grid_total_w = (cell_w * columns) + (margin * (columns - 1));
    float grid_total_h = (cell_h * rows) + (margin * (rows - 1));

    float offset_x = (size.x - grid_total_w) / 2.0f;
    float offset_y = (size.y - grid_total_h) / 2.0f;

    if (p_pos.x < offset_x || p_pos.x > offset_x + grid_total_w ||
        p_pos.y < offset_y || p_pos.y > offset_y + grid_total_h) {
        return -1;
    }

    int col = static_cast<int>((p_pos.x - offset_x) / (cell_w + margin));
    int row = static_cast<int>((p_pos.y - offset_y) / (cell_h + margin));

    if (col < 0 || col >= columns || row < 0 || row >= rows) return -1;

    int idx = (row * columns) + col;
    return (idx >= 0 && idx < total_cells) ? idx : -1;
}

void HeatmapGrid::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        int idx = _get_cell_index_at_pos(mm->get_position());
        if (idx != hovered_index) {
            hovered_index = idx;
            emit_signal("cell_hovered", hovered_index);
            queue_redraw();
        }
    }

    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed()) {
        int idx = _get_cell_index_at_pos(mb->get_position());
        if (idx >= 0 && idx < data.size()) {
            selected_index = idx;
            emit_signal("cell_clicked", idx, mb->get_button_index());
            queue_redraw();
        }
    }

    Control::_gui_input(p_event);
}

void HeatmapGrid::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_MOUSE_EXIT) {
        if (hovered_index != -1) {
            hovered_index = -1;
            emit_signal("cell_hovered", -1);
            queue_redraw();
        }
    } else if (p_what == NOTIFICATION_DRAW) {
        Color bg_color = has_theme_color("background_color", "HeatmapGrid") 
                       ? get_theme_color("background_color", "HeatmapGrid") 
                       : Color(0.10f, 0.10f, 0.13f, 1.0f);

        float margin = has_theme_constant("cell_margin", "HeatmapGrid") 
                     ? static_cast<float>(get_theme_constant("cell_margin", "HeatmapGrid")) 
                     : 1.5f;

        Vector2 size = get_size();
        draw_rect(Rect2(Point2(), size), bg_color);

        int total_cells = data.size();
        if (total_cells == 0 || columns <= 0) return;

        int rows = (total_cells + columns - 1) / columns;
        float available_width = size.x - (margin * (columns + 1));
        float available_height = size.y - (margin * (rows + 1));
        available_width = MAX(available_width, 1.0f);
        available_height = MAX(available_height, 1.0f);

        float cell_w = available_width / static_cast<float>(columns);
        float cell_h = available_height / static_cast<float>(rows);

        if (square_cells) {
            float min_dim = MIN(cell_w, cell_h);
            cell_w = min_dim;
            cell_h = min_dim;
        }

        float grid_total_w = (cell_w * columns) + (margin * (columns - 1));
        float grid_total_h = (cell_h * rows) + (margin * (rows - 1));

        float offset_x = (size.x - grid_total_w) / 2.0f;
        float offset_y = (size.y - grid_total_h) / 2.0f;

        // View-Rect Culling Calculation for High-Capacity Scalability
        Rect2 local_bounds = Rect2(Point2(), size);
        Rect2 visible_window = local_bounds;

        Transform2D global_xform = get_global_transform();
        if (global_xform.determinant() != 0.0f) {
            Rect2 screen_rect = get_viewport_rect();
            Rect2 culled = global_xform.affine_inverse().xform(screen_rect);
            Rect2 intersection = local_bounds.intersection(culled);
            if (intersection.size.x > 0.0f && intersection.size.y > 0.0f) {
                visible_window = intersection;
            }
        }

        int start_row = Math::max(0, static_cast<int>((visible_window.position.y - offset_y) / (cell_h + margin)));
        int end_row = Math::min(rows - 1, static_cast<int>((visible_window.position.y + visible_window.size.y - offset_y) / (cell_h + margin) + 1));

        const uint8_t* read_ptr = data.ptr();
        Color highlight_border = Color(1.0f, 1.0f, 1.0f, 0.95f);
        Color hover_border = Color(0.98f, 0.78f, 0.15f, 1.0f);

        for (int r = start_row; r <= end_row; ++r) {
            float y = offset_y + (r * (cell_h + margin));

            for (int c = 0; c < columns; ++c) {
                int i = (r * columns) + c;
                if (i >= total_cells) break;

                float x = offset_x + (c * (cell_w + margin));
                Rect2 cell_rect(x, y, cell_w, cell_h);

                uint8_t cell_val = read_ptr[i];
                Color cell_color = _get_color_for_value(cell_val);

                draw_rect(cell_rect, cell_color);

                if (i == selected_index) {
                    draw_rect(cell_rect, highlight_border, false, 1.5f);
                } else if (i == hovered_index) {
                    draw_rect(cell_rect, hover_border, false, 1.0f);
                }
            }
        }
    }
}

String HeatmapGrid::_get_tooltip(const Vector2& p_at_position) const {
    int idx = _get_cell_index_at_pos(p_at_position);
    if (idx >= 0 && idx < data.size()) {
        uint8_t val = data[idx];
        int word_idx = idx / 64;
        int bit_idx = idx % 64;

        String state_str = "Unselected (Free)";
        if (val == STATE_SELECTED) state_str = "Selected (Active Write Claim)";
        else if (val == STATE_HOVERED) state_str = "Hovered";
        else if (val == STATE_PENDING) state_str = "Pending Selection";
        else if (val > 3) state_str = "Custom Flag (" + String::num_int64(val) + ")";

        return String("[Shadow Bit #") + String::num_int64(idx) + "]\n" +
               "State: " + state_str + "\n" +
               "Word (uint64): #" + String::num_int64(word_idx) + " (Offset: 0x" + String::num_uint64(word_idx * 8, 16, true) + ")\n" +
               "Bit Position: " + String::num_int64(bit_idx) + " (Mask: 0x" + String::num_uint64(1ULL << bit_idx, 16, true) + ")";
    }
    return Control::_get_tooltip(p_at_position);
}

Vector2 HeatmapGrid::_get_minimum_size() const {
    float margin = has_theme_constant("cell_margin", "HeatmapGrid") 
                 ? static_cast<float>(get_theme_constant("cell_margin", "HeatmapGrid")) 
                 : 1.5f;

    float min_cell_size = 6.0f; 
    if (data.is_empty() || columns <= 0) {
        return Vector2(margin * 2, margin * 2);
    }

    int rows = (data.size() + columns - 1) / columns;
    float min_w = (min_cell_size * columns) + (margin * (columns + 1));
    float min_h = (min_cell_size * rows) + (margin * (rows + 1));

    return Vector2(min_w, min_h);
}

void HeatmapGrid::set_data(const PackedByteArray& p_data) {
    data = p_data;
    queue_redraw();
}
PackedByteArray HeatmapGrid::get_data() const { return data; }

void HeatmapGrid::set_gradient(const Ref<Gradient>& p_gradient) {
    if (color_gradient == p_gradient) return;
    if (color_gradient.is_valid()) color_gradient->disconnect("changed", Callable(this, "queue_redraw"));
    color_gradient = p_gradient;
    if (color_gradient.is_valid()) color_gradient->connect("changed", Callable(this, "queue_redraw"));
    queue_redraw();
}
Ref<Gradient> HeatmapGrid::get_gradient() const { return color_gradient; }

void HeatmapGrid::set_columns(int p_columns) {
    if (columns == p_columns || p_columns <= 0) return;
    columns = p_columns;
    queue_redraw();
    update_minimum_size();
}
int HeatmapGrid::get_columns() const { return columns; }

void HeatmapGrid::set_square_cells(bool p_square) {
    if (square_cells == p_square) return;
    square_cells = p_square;
    queue_redraw();
}
bool HeatmapGrid::get_square_cells() const { return square_cells; }

void HeatmapGrid::set_selected_index(int p_index) {
    if (selected_index == p_index) return;
    selected_index = p_index;
    queue_redraw();
}
int HeatmapGrid::get_selected_index() const { return selected_index; }

void HeatmapGrid::set_hovered_index(int p_index) {
    if (hovered_index == p_index) return;
    hovered_index = p_index;
    queue_redraw();
}
int HeatmapGrid::get_hovered_index() const { return hovered_index; }

void HeatmapGrid::set_value(int p_index, uint8_t p_value) {
    if (p_index < 0 || p_index >= data.size()) return;
    data.set(p_index, p_value);
    queue_redraw();
}

uint8_t HeatmapGrid::get_value(int p_index) const {
    if (p_index < 0 || p_index >= data.size()) return 0;
    return data[p_index];
}

void HeatmapGrid::clear_data() {
    data.clear();
    hovered_index = -1;
    selected_index = -1;
    queue_redraw();
    update_minimum_size();
}

void HeatmapGrid::resize_data(int p_size) {
    if (p_size < 0) return;
    data.resize(p_size);
    queue_redraw();
    update_minimum_size();
}

} // namespace ideam::godot_ext