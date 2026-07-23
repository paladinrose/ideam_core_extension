#include "memory_page_grid.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

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

    ADD_PROPERTY(PropertyInfo(Variant::INT, "page_size"), "set_page_size", "get_page_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "target_columns", PROPERTY_HINT_RANGE, "1,128,1"), "set_target_columns", "get_target_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enforce_square_cells"), "set_enforce_square_cells", "get_enforce_square_cells");
}

MemoryPageGrid::MemoryPageGrid() {
    set_clip_contents(true);
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
    }
    
    queue_redraw();
    update_minimum_size();
}

void MemoryPageGrid::_on_buffer_changed() {
    queue_redraw();
    update_minimum_size();
}

void MemoryPageGrid::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
    else if (p_what == NOTIFICATION_DRAW) {
        // --- 1. Fetch Theme Data ---
        Color bg_color = has_theme_color("background_color", "MemoryPageGrid") 
                       ? get_theme_color("background_color", "MemoryPageGrid") 
                       : Color(0.12f, 0.12f, 0.12f, 1.0f);
                       
        Color allocated_color = has_theme_color("allocated_color", "MemoryPageGrid") 
                              ? get_theme_color("allocated_color", "MemoryPageGrid") 
                              : Color(0.3f, 0.7f, 0.9f, 1.0f);
                              
        Color unallocated_color = has_theme_color("unallocated_color", "MemoryPageGrid") 
                                ? get_theme_color("unallocated_color", "MemoryPageGrid") 
                                : Color(0.2f, 0.2f, 0.2f, 1.0f);
                                
        float margin = has_theme_constant("cell_margin", "MemoryPageGrid") 
                     ? static_cast<float>(get_theme_constant("cell_margin", "MemoryPageGrid")) 
                     : 2.0f;

        Vector2 size = get_size();
        draw_rect(Rect2(Point2(), size), bg_color);

        if (!active_buffer.is_valid() || page_size <= 0 || target_columns <= 0) return;

        // --- 2. Calculate Resource Math ---
        int footprint = active_buffer->calculate_projected_footprint_bytes();
        int total_pages = Math::ceil(static_cast<float>(footprint) / static_cast<float>(page_size));
        
        if (total_pages == 0) return;

        // --- 3. Layout Math ---
        int current_columns = MIN(target_columns, total_pages);
        int rows = Math::ceil(static_cast<float>(total_pages) / static_cast<float>(current_columns));
        
        float available_width = size.x - (margin * (current_columns + 1));
        float available_height = size.y - (margin * (rows + 1));
        
        available_width = MAX(available_width, 1.0f);
        available_height = MAX(available_height, 1.0f);

        float cell_w = available_width / static_cast<float>(current_columns);
        float cell_h = available_height / static_cast<float>(rows);

        if (enforce_square_cells) {
            float min_dim = MIN(cell_w, cell_h);
            cell_w = min_dim;
            cell_h = min_dim;
        }

        // Center alignment calculations
        float grid_total_w = (cell_w * current_columns) + (margin * (current_columns - 1));
        float grid_total_h = (cell_h * rows) + (margin * (rows - 1));
        
        float offset_x = (size.x - grid_total_w) / 2.0f;
        float offset_y = (size.y - grid_total_h) / 2.0f;

        // --- 4. Draw Pages ---
        for (int i = 0; i < total_pages; ++i) {
            int col = i % current_columns;
            int row = i / current_columns;

            float x = offset_x + (col * (cell_w + margin));
            float y = offset_y + (row * (cell_h + margin));

            Rect2 cell_rect(x, y, cell_w, cell_h);
            
            // For now, visualize all projected pages as allocated (since it's a structural view).
            // When migrating to real-time profiling, you can check an active_pages array here.
            draw_rect(cell_rect, allocated_color);
        }
    }
}

Vector2 MemoryPageGrid::_get_minimum_size() const {
    float margin = has_theme_constant("cell_margin", "MemoryPageGrid") 
                 ? static_cast<float>(get_theme_constant("cell_margin", "MemoryPageGrid")) 
                 : 2.0f;
                 
    float min_cell_size = 8.0f; 
    
    int expected_pages = 1;
    if (active_buffer.is_valid() && page_size > 0) {
        int footprint = active_buffer->calculate_projected_footprint_bytes();
        expected_pages = Math::ceil(static_cast<float>(footprint) / static_cast<float>(page_size));
    }
    
    if (expected_pages == 0) return Vector2(margin * 2, margin * 2);
    
    int current_columns = MIN(target_columns, expected_pages);
    int rows = Math::ceil(static_cast<float>(expected_pages) / static_cast<float>(current_columns));
    
    float min_w = (min_cell_size * current_columns) + (margin * (current_columns + 1));
    float min_h = (min_cell_size * rows) + (margin * (rows + 1));
    
    return Vector2(min_w, min_h);
}

// --- Property Setters ---

void MemoryPageGrid::set_page_size(int p_size) {
    if (page_size == p_size || p_size <= 0) return;
    page_size = p_size;
    queue_redraw();
    update_minimum_size();
}
int MemoryPageGrid::get_page_size() const { return page_size; }

void MemoryPageGrid::set_target_columns(int p_columns) {
    if (target_columns == p_columns || p_columns <= 0) return;
    target_columns = p_columns;
    queue_redraw();
    update_minimum_size();
}
int MemoryPageGrid::get_target_columns() const { return target_columns; }

void MemoryPageGrid::set_enforce_square_cells(bool p_enforce) {
    if (enforce_square_cells == p_enforce) return;
    enforce_square_cells = p_enforce;
    queue_redraw();
}
bool MemoryPageGrid::get_enforce_square_cells() const { return enforce_square_cells; }

} // namespace ideam::godot_ext