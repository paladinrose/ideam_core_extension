#include "aos_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void AoSVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &AoSVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &AoSVisualizer::_on_buffer_changed);
}

AoSVisualizer::AoSVisualizer() {
    set_clip_contents(true);
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
    
    queue_redraw();
    update_minimum_size();
}

void AoSVisualizer::_on_buffer_changed() {
    queue_redraw();
    update_minimum_size();
}

void AoSVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
    else if (p_what == NOTIFICATION_DRAW) {
        if (!active_buffer.is_valid()) return;

        TypedArray<Dictionary> columns = active_buffer->get_columns();
        if (columns.is_empty()) return;

        // --- 1. Fetch Theme Styling ---
        Color data_color = has_theme_color("data_color", "AoSVisualizer") 
                         ? get_theme_color("data_color", "AoSVisualizer") 
                         : Color(0.2f, 0.6f, 0.86f, 1.0f); // Default Blue
                         
        Color padding_color = has_theme_color("padding_color", "AoSVisualizer") 
                            ? get_theme_color("padding_color", "AoSVisualizer") 
                            : Color(0.9f, 0.3f, 0.2f, 1.0f); // Default Warning Red
                            
        Color border_color = has_theme_color("border_color", "AoSVisualizer") 
                           ? get_theme_color("border_color", "AoSVisualizer") 
                           : Color(0.1f, 0.1f, 0.1f, 1.0f);
                           
        Color text_color = has_theme_color("text_color", "AoSVisualizer") 
                         ? get_theme_color("text_color", "AoSVisualizer") 
                         : Color(1.0f, 1.0f, 1.0f, 1.0f);

        Ref<Font> font = get_theme_font("font", "Label");
        int font_size = get_theme_font_size("font_size", "Label");
        if (font_size <= 0) font_size = 14;

        // --- 2. Calculate Layout & Alignment (STD140/430 Simulation) ---
        Vector<RenderBlock> blocks;
        int current_offset = 0;
        int max_alignment = 1;

        for (int i = 0; i < columns.size(); ++i) {
            Dictionary col = columns[i];
            
            int size = col.has("size") ? static_cast<int>(col["size"]) : 4;
            String name = col.has("name") ? String(col["name"]) : "Col " + String::num_int64(i);
            
            // Assume alignment is equal to size if not explicitly provided, capped at 16 (vec4 limit)
            int align = col.has("alignment") ? static_cast<int>(col["alignment"]) : size;
            align = CLAMP(align, 1, 16); 
            
            max_alignment = MAX(max_alignment, align);

            // Check for alignment gap
            int padding = (align - (current_offset % align)) % align;
            if (padding > 0) {
                blocks.push_back({"Pad", padding, true});
                current_offset += padding;
            }

            blocks.push_back({name, size, false});
            current_offset += size;
        }

        // Apply trailing structural padding based on the largest alignment requirement
        int trailing_pad = (max_alignment - (current_offset % max_alignment)) % max_alignment;
        if (trailing_pad > 0) {
            blocks.push_back({"Pad", trailing_pad, true});
            current_offset += trailing_pad;
        }

        int total_stride_bytes = current_offset;
        if (total_stride_bytes == 0) return;

        // --- 3. Draw the Stride ---
        Vector2 size_rect = get_size();
        float margin_y = 10.0f; // Give it breathing room top and bottom
        float bar_height = size_rect.y - (margin_y * 2.0f);
        if (bar_height <= 0) bar_height = size_rect.y;

        float pixels_per_byte = size_rect.x / static_cast<float>(total_stride_bytes);
        float current_x = 0.0f;

        for (int i = 0; i < blocks.size(); ++i) {
            const RenderBlock& block = blocks[i];
            float block_width = block.size_bytes * pixels_per_byte;
            
            Rect2 rect(current_x, margin_y, block_width, bar_height);
            
            // Draw background fill
            draw_rect(rect, block.is_padding ? padding_color : data_color);
            // Draw border
            draw_rect(rect, border_color, false, 2.0f);

            // Try to draw text if the block is wide enough
            String label = block.name + String("\n(") + String::num_int64(block.size_bytes) + String("B)");
            
            if (font.is_valid()) {
                Vector2 string_size = font->get_string_size(label, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                if (string_size.x < (block_width - 4.0f)) {
                    // Center the text in the block
                    Vector2 text_pos = rect.position + (rect.size / 2.0f);
                    text_pos.y += (string_size.y / 4.0f); // Adjust baseline visually
                    text_pos.x -= (string_size.x / 2.0f);
                    
                    draw_string(font, text_pos, label, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, text_color);
                } else if (string_size.x < block_width && block.size_bytes > 0) {
                    // Fallback to just drawing the size if the name won't fit
                    String short_label = String::num_int64(block.size_bytes) + "B";
                    Vector2 short_size = font->get_string_size(short_label, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
                    if (short_size.x < block_width) {
                        Vector2 text_pos = rect.position + (rect.size / 2.0f);
                        text_pos.y += (short_size.y / 4.0f);
                        text_pos.x -= (short_size.x / 2.0f);
                        draw_string(font, text_pos, short_label, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, text_color);
                    }
                }
            }

            current_x += block_width;
        }
    }
}

Vector2 AoSVisualizer::_get_minimum_size() const {
    return Vector2(100.0f, 40.0f); // Provide a sensible default minimum
}

} // namespace ideam::godot_ext