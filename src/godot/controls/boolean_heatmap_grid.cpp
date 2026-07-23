#include "boolean_heatmap_grid.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace ideam::godot_ext {

void BooleanHeatmapGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_data", "data"), &BooleanHeatmapGrid::set_data);
    ClassDB::bind_method(D_METHOD("get_data"), &BooleanHeatmapGrid::get_data);

    ClassDB::bind_method(D_METHOD("set_columns", "columns"), &BooleanHeatmapGrid::set_columns);
    ClassDB::bind_method(D_METHOD("get_columns"), &BooleanHeatmapGrid::get_columns);

    ClassDB::bind_method(D_METHOD("set_square_cells", "square_cells"), &BooleanHeatmapGrid::set_square_cells);
    ClassDB::bind_method(D_METHOD("get_square_cells"), &BooleanHeatmapGrid::get_square_cells);
    
    ClassDB::bind_method(D_METHOD("set_value", "index", "value"), &BooleanHeatmapGrid::set_value);
    ClassDB::bind_method(D_METHOD("get_value", "index"), &BooleanHeatmapGrid::get_value);
    ClassDB::bind_method(D_METHOD("clear_data"), &BooleanHeatmapGrid::clear_data);
    ClassDB::bind_method(D_METHOD("resize_data", "size"), &BooleanHeatmapGrid::resize_data);

    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,1024,1"), "set_columns", "get_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "square_cells"), "set_square_cells", "get_square_cells");
}

BooleanHeatmapGrid::BooleanHeatmapGrid() {
    set_clip_contents(true);
}

void BooleanHeatmapGrid::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
    else if (p_what == NOTIFICATION_DRAW) {
        // --- 1. Fetch Theme Data ---
        Color bg_color = has_theme_color("background_color", "BooleanHeatmapGrid") 
                       ? get_theme_color("background_color", "BooleanHeatmapGrid") 
                       : Color(0.1f, 0.1f, 0.1f, 1.0f);
                       
        Color active_color = has_theme_color("active_color", "BooleanHeatmapGrid") 
                         ? get_theme_color("active_color", "BooleanHeatmapGrid") 
                         : Color(0.2f, 0.8f, 0.4f, 1.0f);
                         
        Color inactive_color = has_theme_color("inactive_color", "BooleanHeatmapGrid") 
                           ? get_theme_color("inactive_color", "BooleanHeatmapGrid") 
                           : Color(0.25f, 0.25f, 0.25f, 1.0f);
                           
        float margin = has_theme_constant("cell_margin", "BooleanHeatmapGrid") 
                     ? static_cast<float>(get_theme_constant("cell_margin", "BooleanHeatmapGrid")) 
                     : 2.0f;

        Vector2 size = get_size();
        
        // Draw standard background behind the grid
        draw_rect(Rect2(Point2(), size), bg_color);

        int total_cells = data.size();
        if (total_cells == 0 || columns <= 0) return;

        // --- 2. Calculate Layout & Geometry ---
        int rows = (total_cells + columns - 1) / columns; // Ceiling division
        
        float available_width = size.x - (margin * (columns + 1));
        float available_height = size.y - (margin * (rows + 1));
        
        // Prevent negative cell sizing if margins are too large for the rect
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

        // --- 3. Draw Cells ---
        const uint8_t* read_ptr = data.ptr();
        
        for (int i = 0; i < total_cells; ++i) {
            int current_col = i % columns;
            int current_row = i / columns;

            float x = offset_x + (current_col * (cell_w + margin));
            float y = offset_y + (current_row * (cell_h + margin));

            Rect2 cell_rect(x, y, cell_w, cell_h);
            
            bool is_active = read_ptr[i] > 0;
            draw_rect(cell_rect, is_active ? active_color : inactive_color);
        }
    }
}

Vector2 BooleanHeatmapGrid::_get_minimum_size() const {
    float margin = has_theme_constant("cell_margin", "BooleanHeatmapGrid") 
                 ? static_cast<float>(get_theme_constant("cell_margin", "BooleanHeatmapGrid")) 
                 : 2.0f;
                 
    // Enforce a tiny minimum size per cell just to ensure the layout engine doesn't crush it entirely
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

void BooleanHeatmapGrid::set_data(const PackedByteArray& p_data) {
    data = p_data;
    queue_redraw();
}

PackedByteArray BooleanHeatmapGrid::get_data() const {
    return data;
}

void BooleanHeatmapGrid::set_columns(int p_columns) {
    if (columns == p_columns || p_columns <= 0) return;
    columns = p_columns;
    queue_redraw();
    update_minimum_size(); // Rows count will change, alert the layout engine
}

int BooleanHeatmapGrid::get_columns() const {
    return columns;
}

void BooleanHeatmapGrid::set_square_cells(bool p_square) {
    if (square_cells == p_square) return;
    square_cells = p_square;
    queue_redraw();
}

bool BooleanHeatmapGrid::get_square_cells() const {
    return square_cells;
}

void BooleanHeatmapGrid::set_value(int p_index, bool p_value) {
    if (p_index < 0 || p_index >= data.size()) return;
    
    data.set(p_index, p_value ? 1 : 0);
    queue_redraw();
}

bool BooleanHeatmapGrid::get_value(int p_index) const {
    if (p_index < 0 || p_index >= data.size()) return false;
    return data[p_index] > 0;
}

void BooleanHeatmapGrid::clear_data() {
    data.clear();
    queue_redraw();
    update_minimum_size();
}

void BooleanHeatmapGrid::resize_data(int p_size) {
    if (p_size < 0) return;
    data.resize(p_size);
    queue_redraw();
    update_minimum_size();
}

} // namespace ideam::godot_ext