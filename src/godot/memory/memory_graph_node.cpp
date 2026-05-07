#include "memory_graph_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/font.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("update_telemetry", "inspector"), &MemoryGraphNode::update_telemetry);
    ClassDB::bind_method(D_METHOD("_on_inspect_memory_pressed"), &MemoryGraphNode::_on_inspect_memory_pressed);
    ClassDB::bind_method(D_METHOD("get_port_signature", "port_idx", "is_output"), &MemoryGraphNode::get_port_signature);
    ClassDB::bind_method(D_METHOD("set_header_state", "state"), &MemoryGraphNode::set_header_state);
    ClassDB::bind_method(D_METHOD("update_memory_port", "slot_index", "is_left", "layout_type"), &MemoryGraphNode::update_memory_port);

    ADD_SIGNAL(MethodInfo("inspect_memory_requested", PropertyInfo(Variant::OBJECT, "inspector", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGrantInspector")));
    ADD_SIGNAL(MethodInfo("buffer_names_requested", 
        PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGraphNode"),
        PropertyInfo(Variant::ARRAY, "buffer_ids", PROPERTY_HINT_ARRAY_TYPE, "int")));
    
}

MemoryGraphNode::MemoryGraphNode() {
}

void MemoryGraphNode::receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) {
    
}

void MemoryGraphNode::_build_ui() {
    IdeamGraphNode::_build_ui(); // Call base setup

    // Pre-create the telemetry button, but keep it hidden until runtime data arrives
    inspect_memory_btn = memnew(Button);
    inspect_memory_btn->set_text("No Memory Grant");
    inspect_memory_btn->set_disabled(true);
    inspect_memory_btn->connect("pressed", Callable(this, "_on_inspect_memory_pressed"));
    add_child(inspect_memory_btn);
}

void MemoryGraphNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAW) {
        // 1. Draw Layout Header States (Visualizing allocation integrity)
        Ref<StyleBox> header_style;
        if (header_state == HEADER_ERROR) {
            header_style = get_theme_stylebox("layout_header_error");
        } else if (header_state == HEADER_VALID) {
            header_style = get_theme_stylebox("layout_header_valid");
        }

        if (header_style.is_valid()) {
            // Estimate title bar area. Godot 4 GraphNodes have a dedicated titlebar HBox.
            // We draw over the top ~30 pixels to tint the allocation header.
            float title_height = 30.0f; 
            draw_style_box(header_style, Rect2(Point2(0, 0), Size2(get_size().width, title_height)));
        }

        // 2. Draw Telemetry Overlay (Badge Icon)
        Ref<Texture2D> badge_icon = _get_badge_icon_for_telemetry(telemetry_state);
        if (badge_icon.is_valid()) {
            // Draw in the top-right corner, inset slightly
            Vector2 badge_pos = Vector2(get_size().width - badge_icon->get_width() - 10, 5);
            draw_texture(badge_icon, badge_pos);
        }

        // 3. Draw Telemetry Text directly onto the node canvas if Active/Dirty
        if (latest_grant_snapshot.is_valid() && (telemetry_state == TELEMETRY_ACTIVE || telemetry_state == TELEMETRY_DIRTY)) {
            Ref<Font> font = get_theme_font("title_font");
            int font_size = get_theme_font_size("title_font_size") - 2; // Slightly smaller than title
            
            if (font.is_valid() && latest_grant_snapshot->get_part_count() > 0) {
                // For simplicity on the canvas, we visualize the telemetry of the primary/first part.
                // Complete details are handled by the inspector side-panel via the button.
                Dictionary primary_part = latest_grant_snapshot->get_part_snapshot(0);
                
                String telemetry_str = String("Cap: ") + String::num_int64(primary_part.get("capacity", 0)) +
                                       String(" | Elem: ") + String::num_int64(primary_part.get("element_count", 0));
                
                // Draw text near the bottom of the node
                Vector2 text_pos = Vector2(10, get_size().height - 15);
                draw_string(font, text_pos, telemetry_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, get_theme_color("font_color"));
            }
        }
    }
}

// --- Theme Mapping Helpers ---

Ref<Texture2D> MemoryGraphNode::_get_icon_for_layout(core::BufferLayoutType p_layout) const {
    // Map structural memory layout directly to port shapes
    if (static_cast<uint16_t>(p_layout) & static_cast<uint16_t>(core::BufferLayoutType::FLAT | core::BufferLayoutType::AOS | core::BufferLayoutType::SOA)) {
        return get_theme_icon("port_shape_contiguous");
    } 
    else if (static_cast<uint16_t>(p_layout) & static_cast<uint16_t>(core::BufferLayoutType::SPARSE_SET | core::BufferLayoutType::TILED_SOA)) {
        return get_theme_icon("port_shape_fragmented");
    }
    else if (static_cast<uint16_t>(p_layout) & static_cast<uint16_t>(core::BufferLayoutType::RING | core::BufferLayoutType::PAGED)) {
        return get_theme_icon("port_shape_virtual");
    }
    
    return get_theme_icon("port_shape_error");
}

Ref<Texture2D> MemoryGraphNode::_get_badge_icon_for_telemetry(TelemetryBadgeState p_state) const {
    switch (p_state) {
        case TELEMETRY_ACTIVE:   return get_theme_icon("telemetry_badge_active");
        case TELEMETRY_DIRTY:    return get_theme_icon("telemetry_badge_dirty");
        case TELEMETRY_ERROR:    return get_theme_icon("telemetry_badge_error");
        case TELEMETRY_INACTIVE: 
        default:                 return get_theme_icon("telemetry_badge_inactive");
    }
}

// --- Tier 2: Public Mutations ---

void MemoryGraphNode::update_memory_port(int p_slot_index, bool p_is_left, core::BufferLayoutType p_layout) {
    Ref<Texture2D> shape_icon = _get_icon_for_layout(p_layout);

    // Persist existing settings, only overwrite the icon
    bool enable_left  = is_slot_enabled_left(p_slot_index);
    int  type_left    = get_slot_type_left(p_slot_index);
    Color color_left  = get_slot_color_left(p_slot_index);
    Ref<Texture2D> icon_left = p_is_left ? shape_icon : get_slot_custom_icon_left(p_slot_index);

    bool enable_right = is_slot_enabled_right(p_slot_index);
    int  type_right   = get_slot_type_right(p_slot_index);
    Color color_right = get_slot_color_right(p_slot_index);
    Ref<Texture2D> icon_right = !p_is_left ? shape_icon : get_slot_custom_icon_right(p_slot_index);

    set_slot(p_slot_index, 
             enable_left, type_left, color_left, 
             enable_right, type_right, color_right, 
             icon_left, icon_right);
}

void MemoryGraphNode::set_header_state(LayoutHeaderState p_state) {
    if (header_state == p_state) return;
    header_state = p_state;
    queue_redraw();
}

void MemoryGraphNode::update_telemetry(const Ref<MemoryGrantInspector>& p_inspector) {
    latest_grant_snapshot = p_inspector;
    
    if (latest_grant_snapshot.is_valid()) {
        if (latest_grant_snapshot->has_error()) {
            telemetry_state = TELEMETRY_ERROR;
            inspect_memory_btn->set_disabled(true);
            inspect_memory_btn->set_text("Grant Error (Dangling Pointer)");
            set_self_modulate(Color(1.0f, 0.5f, 0.5f)); 
        } 
        else if (latest_grant_snapshot->is_dirty()) {
            telemetry_state = TELEMETRY_DIRTY;
            inspect_memory_btn->set_disabled(false);
            inspect_memory_btn->set_text("Inspect Grant (Stale Data)");
            set_self_modulate(Color(1.0f, 0.9f, 0.5f)); 
        }
        else if (latest_grant_snapshot->is_active()) {
            telemetry_state = TELEMETRY_ACTIVE;
            inspect_memory_btn->set_disabled(false);
            inspect_memory_btn->set_text(String("Inspect Grant (") + String::num_int64(latest_grant_snapshot->get_part_count()) + " parts)");
            set_self_modulate(Color(0.8f, 1.0f, 0.8f)); 
        } 
        else {
            telemetry_state = TELEMETRY_INACTIVE;
            inspect_memory_btn->set_disabled(true);
            inspect_memory_btn->set_text("Grant Inactive");
            set_self_modulate(Color(1.0f, 1.0f, 1.0f));
        }
    } else {
        telemetry_state = TELEMETRY_INACTIVE;
        inspect_memory_btn->set_disabled(true);
        inspect_memory_btn->set_text("No Memory Grant");
        set_self_modulate(Color(1.0f, 1.0f, 1.0f));
    }

    queue_redraw();
}

void MemoryGraphNode::_on_inspect_memory_pressed() {
    if (latest_grant_snapshot.is_valid()) {
        emit_signal("inspect_memory_requested", latest_grant_snapshot);
    }
}

void MemoryGraphNode::register_port_signature(int p_port_idx, bool p_is_output, uint32_t p_trait_mask) {
    if (p_is_output) {
        output_port_signatures[p_port_idx] = p_trait_mask;
    } else {
        input_port_signatures[p_port_idx] = p_trait_mask;
    }
}

uint32_t MemoryGraphNode::get_port_signature(int p_port_idx, bool p_is_output) const {
    if (p_is_output) {
        auto it = output_port_signatures.find(p_port_idx);
        return it != output_port_signatures.end() ? it->second : TRAIT_NONE;
    } else {
        auto it = input_port_signatures.find(p_port_idx);
        return it != input_port_signatures.end() ? it->second : TRAIT_NONE;
    }
}

} // namespace ideam::godot_ext