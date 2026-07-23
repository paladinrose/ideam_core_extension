#include "soa_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void SoAVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &SoAVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &SoAVisualizer::_on_buffer_changed);
}

SoAVisualizer::SoAVisualizer() {
    set_clip_contents(true);
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
    
    queue_redraw();
    update_minimum_size();
}

void SoAVisualizer::_on_buffer_changed() {
    queue_redraw();
    update_minimum_size();
}

void SoAVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
    else if (p_what == NOTIFICATION_DRAW) {
        if (!active_buffer.is_valid()) return;

        TypedArray<Dictionary> columns = active_buffer->get_columns();
        int num_columns = columns.size();
        if (num_columns == 0) return;

        int max_elements = active_buffer->get_max_elements();
        if (max_elements <= 0) return;

        int alignment = active_buffer->get_alignment(); 
        if (alignment <= 0) alignment = 64; // Fallback to standard cache line

        // --- 1. Fetch Theme Styling ---
        Color even_ribbon_color = has_theme_color("even_ribbon_color", "SoAVisualizer") 
                                ? get_theme_color("even_ribbon_color", "SoAVisualizer") 
                                : Color(0.2f, 0.4f, 0.6f, 1.0f);
                         
        Color odd_ribbon_color = has_theme_color("odd_ribbon_color", "SoAVisualizer") 
                               ? get_theme_color("odd_ribbon_color", "SoAVisualizer") 
                               : Color(0.15f, 0.35f, 0.55f, 1.0f);
                               
        Color boundary_color = has_theme_color("boundary_color", "SoAVisualizer") 
                             ? get_theme_color("boundary_color", "SoAVisualizer") 
                             : Color(0.9f, 0.8f, 0.2f, 0.6f); // Yellowish transparent line
                             
        Color text_color = has_theme_color("text_color", "SoAVisualizer") 
                         ? get_theme_color("text_color", "SoAVisualizer") 
                         : Color(1.0f, 1.0f, 1.0f, 1.0f);

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 14;

        // --- 2. Calculate Layout ---
        Vector2 size_rect = get_size();
        float ribbon_spacing = 6.0f;
        float total_spacing = ribbon_spacing * (num_columns - 1);
        float ribbon_height = (size_rect.y - total_spacing) / static_cast<float>(num_columns);
        
        // Prevent squishing if container is too small
        if (ribbon_height < 20.0f) ribbon_height = 20.0f; 

        // --- 3. Draw Ribbons ---
        for (int i = 0; i < num_columns; ++i) {
            Dictionary col = columns[i];
            
            int col_size = col.has("size") ? static_cast<int>(col["size"]) : 4;
            String name = col.has("name") ? String(col["name"]) : String("Col ") + String::num_int64(i);
            
            float y_pos = i * (ribbon_height + ribbon_spacing);
            Rect2 ribbon_rect(0.0f, y_pos, size_rect.x, ribbon_height);
            
            // Draw background fill
            Color fill_color = (i % 2 == 0) ? even_ribbon_color : odd_ribbon_color;
            draw_rect(ribbon_rect, fill_color);

            // --- 4. Draw Cache/SIMD Boundaries ---
            // Calculate how many elements fit inside the alignment threshold
            int elements_per_boundary = MAX(1, alignment / col_size); 
            
            // Limit drawing density so we don't draw thousands of overlapping lines on huge buffers
            int boundary_count = max_elements / elements_per_boundary;
            
            if (boundary_count < 1000) { 
                float pixels_per_element = size_rect.x / static_cast<float>(max_elements);
                float pixels_per_boundary = pixels_per_element * elements_per_boundary;
                
                for (float x = pixels_per_boundary; x < size_rect.x; x += pixels_per_boundary) {
                    draw_line(Vector2(x, y_pos), Vector2(x, y_pos + ribbon_height), boundary_color, 1.0f);
                }
            } else {
                // If the buffer is massive, draw a warning instead of freezing the renderer
                String warn_label = String("Buffer too large to render boundaries.");
                draw_string(font, ribbon_rect.position + Vector2(10, ribbon_height - 4), warn_label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size - 4, Color(1, 0, 0, 0.8));
            }

            // --- 5. Draw Labels ---
            // Construct string safely to avoid operator+ ambiguity
            String label = name + String(" (") + String::num_int64(col_size) + String("B/elem)");
            
            if (font.is_valid()) {
                Vector2 text_pos = ribbon_rect.position + Vector2(10.0f, (ribbon_height / 2.0f) + (font_size / 3.0f));
                
                // Add a slight drop shadow for readability over the lines
                draw_string(font, text_pos + Vector2(1, 1), label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0, 0, 0, 0.8f));
                draw_string(font, text_pos, label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_color);
            }
        }
    }
}

Vector2 SoAVisualizer::_get_minimum_size() const {
    int num_columns = 1;
    if (active_buffer.is_valid()) {
        num_columns = active_buffer->get_columns().size();
        if (num_columns == 0) num_columns = 1;
    }
    
    float min_ribbon_height = 20.0f;
    float ribbon_spacing = 6.0f;
    float total_h = (min_ribbon_height * num_columns) + (ribbon_spacing * (num_columns - 1));
    
    return Vector2(100.0f, total_h); 
}

} // namespace ideam::godot_ext