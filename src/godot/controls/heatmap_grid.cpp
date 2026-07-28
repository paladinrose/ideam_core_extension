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

    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "color_gradient", PROPERTY_HINT_RESOURCE_TYPE, "Gradient"), "set_gradient", "get_gradient");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,1024,1"), "set_columns", "get_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "square_cells"), "set_square_cells", "get_square_cells");
}

HeatmapGrid::HeatmapGrid() {
    set_clip_contents(true);
}

Color HeatmapGrid::_get_color_for_value(uint8_t p_value) const {
    // 1. Preferred: Utilize the Gradient if it exists
    if (color_gradient.is_valid()) {
        // Normalize the byte value (0-255) to a float (0.0-1.0)
        float offset = static_cast<float>(p_value) / 255.0f;
        return color_gradient->sample(offset);
    }

    // 2. Fallback: Dynamically check the Theme for specific state colors
    String color_name = "state_" + String::num_int64(p_value) + "_color";
    if (has_theme_color(color_name, "HeatmapGrid")) {
        return get_theme_color(color_name, "HeatmapGrid");
    }

    // 3. Last Resort: Default styling (0 is inactive/background, >0 is active)
    if (p_value == 0) {
        return has_theme_color("inactive_color", "HeatmapGrid") 
            ? get_theme_color("inactive_color", "HeatmapGrid") 
            : Color(0.25f, 0.25f, 0.25f, 1.0f);
    } else {
        // Give unknown active states a generic "heatmap" scale based on value intensity
        float intensity = static_cast<float>(p_value) / 255.0f;
        return Color(intensity, intensity * 0.5f, 0.2f, 1.0f);
    }
}

void HeatmapGrid::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
    else if (p_what == NOTIFICATION_DRAW) {
        Color bg_color = has_theme_color("background_color", "HeatmapGrid") 
                       ? get_theme_color("background_color", "HeatmapGrid") 
                       : Color(0.1f, 0.1f, 0.1f, 1.0f);
                       
        float margin = has_theme_constant("cell_margin", "HeatmapGrid") 
                     ? static_cast<float>(get_theme_constant("cell_margin", "HeatmapGrid")) 
                     : 2.0f;

        Vector2 size = get_size();
        
        // Draw standard background behind the grid
        draw_rect(Rect2(Point2(), size), bg_color);

        int total_cells = data.size();
        if (total_cells == 0 || columns <= 0) return;

        // Calculate Layout & Geometry
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

        // Center the grid within the control if square cells caused it to shrink
        float grid_total_w = (cell_w * columns) + (margin * (columns - 1));
        float grid_total_h = (cell_h * rows) + (margin * (rows - 1));
        
        float offset_x = (size.x - grid_total_w) / 2.0f;
        float offset_y = (size.y - grid_total_h) / 2.0f;

        // Draw Cells
        const uint8_t* read_ptr = data.ptr();
        
        for (int i = 0; i < total_cells; ++i) {
            int current_col = i % columns;
            int current_row = i / columns;

            float x = offset_x + (current_col * (cell_w + margin));
            float y = offset_y + (current_row * (cell_h + margin));

            Rect2 cell_rect(x, y, cell_w, cell_h);
            
            Color cell_color = _get_color_for_value(read_ptr[i]);
            draw_rect(cell_rect, cell_color);
        }
    }
}

Vector2 HeatmapGrid::_get_minimum_size() const {
    float margin = has_theme_constant("cell_margin", "HeatmapGrid") 
                 ? static_cast<float>(get_theme_constant("cell_margin", "HeatmapGrid")) 
                 : 2.0f;
                 
    float min_cell_size = 4.0f; 
    
    if (data.is_empty() || columns <= 0) {
        return Vector2(margin * 2, margin * 2);
    }
    
    int rows = (data.size() + columns - 1) / columns;
    
    float min_w = (min_cell_size * columns) + (margin * (columns + 1));
    float min_h = (min_cell_size * rows) + (margin * (rows + 1));
    
    return Vector2(min_w, min_h);
}

// --- Standard Setters and Data Management ---

void HeatmapGrid::set_data(const PackedByteArray& p_data) {
    data = p_data;
    queue_redraw();
}

PackedByteArray HeatmapGrid::get_data() const {
    return data;
}

void HeatmapGrid::set_gradient(const Ref<Gradient>& p_gradient) {
    if (color_gradient == p_gradient) return;
    
    if (color_gradient.is_valid() && color_gradient->is_connected("changed", Callable(this, "queue_redraw"))) {
        color_gradient->disconnect("changed", Callable(this, "queue_redraw"));
    }
    
    color_gradient = p_gradient;
    
    if (color_gradient.is_valid()) {
        color_gradient->connect("changed", Callable(this, "queue_redraw"));
    }
    
    queue_redraw();
}

Ref<Gradient> HeatmapGrid::get_gradient() const {
    return color_gradient;
}

void HeatmapGrid::set_columns(int p_columns) {
    if (columns == p_columns || p_columns <= 0) return;
    columns = p_columns;
    queue_redraw();
    update_minimum_size();
}

int HeatmapGrid::get_columns() const {
    return columns;
}

void HeatmapGrid::set_square_cells(bool p_square) {
    if (square_cells == p_square) return;
    square_cells = p_square;
    queue_redraw();
}

bool HeatmapGrid::get_square_cells() const {
    return square_cells;
}

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