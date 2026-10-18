#include "circular_gauge.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace ideam::godot_ext {

void CircularGauge::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_direction", "direction"), &CircularGauge::set_direction);
    ClassDB::bind_method(D_METHOD("get_direction"), &CircularGauge::get_direction);
    
    ClassDB::bind_method(D_METHOD("set_marker_count", "count"), &CircularGauge::set_marker_count);
    ClassDB::bind_method(D_METHOD("get_marker_count"), &CircularGauge::get_marker_count);

    ClassDB::bind_method(D_METHOD("set_start_angle", "angle"), &CircularGauge::set_start_angle);
    ClassDB::bind_method(D_METHOD("get_start_angle"), &CircularGauge::get_start_angle);
    
    ClassDB::bind_method(D_METHOD("set_end_angle", "angle"), &CircularGauge::set_end_angle);
    ClassDB::bind_method(D_METHOD("get_end_angle"), &CircularGauge::get_end_angle);
    
    ClassDB::bind_method(D_METHOD("set_min_value", "value"), &CircularGauge::set_min_value);
    ClassDB::bind_method(D_METHOD("get_min_value"), &CircularGauge::get_min_value);
    
    ClassDB::bind_method(D_METHOD("set_max_value", "value"), &CircularGauge::set_max_value);
    ClassDB::bind_method(D_METHOD("get_max_value"), &CircularGauge::get_max_value);
    
    ClassDB::bind_method(D_METHOD("set_value_1", "value"), &CircularGauge::set_value_1);
    ClassDB::bind_method(D_METHOD("get_value_1"), &CircularGauge::get_value_1);
    
    ClassDB::bind_method(D_METHOD("set_value_2", "value"), &CircularGauge::set_value_2);
    ClassDB::bind_method(D_METHOD("get_value_2"), &CircularGauge::get_value_2);

    ClassDB::bind_method(D_METHOD("set_draw_as_band", "draw_as_band"), &CircularGauge::set_draw_as_band);
    ClassDB::bind_method(D_METHOD("get_draw_as_band"), &CircularGauge::get_draw_as_band);
    
    ClassDB::bind_method(D_METHOD("set_width_curve", "curve"), &CircularGauge::set_width_curve);
    ClassDB::bind_method(D_METHOD("get_width_curve"), &CircularGauge::get_width_curve);

    ClassDB::bind_method(D_METHOD("set_is_wrapped", "wrapped"), &CircularGauge::set_is_wrapped);
    ClassDB::bind_method(D_METHOD("get_is_wrapped"), &CircularGauge::get_is_wrapped);

    ClassDB::bind_method(D_METHOD("set_is_full", "full"), &CircularGauge::set_is_full);
    ClassDB::bind_method(D_METHOD("get_is_full"), &CircularGauge::get_is_full);

    ClassDB::bind_method(D_METHOD("set_is_overflowing", "overflowing"), &CircularGauge::set_is_overflowing);
    ClassDB::bind_method(D_METHOD("get_is_overflowing"), &CircularGauge::get_is_overflowing);

    ClassDB::bind_method(D_METHOD("set_slot_count", "slots"), &CircularGauge::set_slot_count);
    ClassDB::bind_method(D_METHOD("get_slot_count"), &CircularGauge::get_slot_count);

    ClassDB::bind_method(D_METHOD("set_draw_slot_ticks", "draw"), &CircularGauge::set_draw_slot_ticks);
    ClassDB::bind_method(D_METHOD("get_draw_slot_ticks"), &CircularGauge::get_draw_slot_ticks);

    ClassDB::bind_method(D_METHOD("set_draw_head_tail_markers", "draw"), &CircularGauge::set_draw_head_tail_markers);
    ClassDB::bind_method(D_METHOD("get_draw_head_tail_markers"), &CircularGauge::get_draw_head_tail_markers);

    ClassDB::bind_method(D_METHOD("set_layer_colors", "colors"), &CircularGauge::set_layer_colors);
    ClassDB::bind_method(D_METHOD("get_layer_colors"), &CircularGauge::get_layer_colors);

    ClassDB::bind_method(D_METHOD("set_custom_bg_color", "color"), &CircularGauge::set_custom_bg_color);
    ClassDB::bind_method(D_METHOD("get_custom_bg_color"), &CircularGauge::get_custom_bg_color);

    ClassDB::bind_method(D_METHOD("set_custom_fill_color", "color"), &CircularGauge::set_custom_fill_color);
    ClassDB::bind_method(D_METHOD("get_custom_fill_color"), &CircularGauge::get_custom_fill_color);

    ClassDB::bind_method(D_METHOD("set_custom_overflow_color", "color"), &CircularGauge::set_custom_overflow_color);
    ClassDB::bind_method(D_METHOD("get_custom_overflow_color"), &CircularGauge::get_custom_overflow_color);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_ENUM, "Clockwise,Counter-Clockwise"), "set_direction", "get_direction");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "marker_count", PROPERTY_HINT_RANGE, "1,2"), "set_marker_count", "get_marker_count");
    
    ADD_GROUP("Angles & Values", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "start_angle"), "set_start_angle", "get_start_angle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "end_angle"), "set_end_angle", "get_end_angle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_value"), "set_min_value", "get_min_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value"), "set_max_value", "get_max_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value_1"), "set_value_1", "get_value_1");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value_2"), "set_value_2", "get_value_2");

    ADD_GROUP("Drawing Options", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_as_band"), "set_draw_as_band", "get_draw_as_band");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "width_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_width_curve", "get_width_curve");

    ADD_GROUP("Ring Buffer Options", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_wrapped"), "set_is_wrapped", "get_is_wrapped");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_full"), "set_is_full", "get_is_full");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_overflowing"), "set_is_overflowing", "get_is_overflowing");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "slot_count"), "set_slot_count", "get_slot_count");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_slot_ticks"), "set_draw_slot_ticks", "get_draw_slot_ticks");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_head_tail_markers"), "set_draw_head_tail_markers", "get_draw_head_tail_markers");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "layer_colors"), "set_layer_colors", "get_layer_colors");

    ADD_GROUP("Custom Colors", "custom_");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "custom_bg_color"), "set_custom_bg_color", "get_custom_bg_color");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "custom_fill_color"), "set_custom_fill_color", "get_custom_fill_color");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "custom_overflow_color"), "set_custom_overflow_color", "get_custom_overflow_color");
}

CircularGauge::CircularGauge() {
    set_clip_contents(true);
    set_mouse_filter(MOUSE_FILTER_PASS);
}

void CircularGauge::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_DRAW) {
        // --- 1. Resolve Theme Palette ---
        Color bg_color = (custom_bg_color.a > 0.0f) ? custom_bg_color
                       : (has_theme_color("background_color", "CircularGauge") ? get_theme_color("background_color", "CircularGauge") : Color(0.14f, 0.15f, 0.18f, 0.85f));

        Color fill_color = (custom_fill_color.a > 0.0f) ? custom_fill_color
                         : (has_theme_color("fill_color", "CircularGauge") ? get_theme_color("fill_color", "CircularGauge") : Color(0.12f, 0.65f, 0.85f, 1.0f));

        Color overflow_color = (custom_overflow_color.a > 0.0f) ? custom_overflow_color
                             : (has_theme_color("overflow_color", "CircularGauge") ? get_theme_color("overflow_color", "CircularGauge") : Color(0.92f, 0.22f, 0.22f, 0.95f));

        Color tick_color = has_theme_color("tick_color", "CircularGauge")
                         ? get_theme_color("tick_color", "CircularGauge")
                         : Color(1.0f, 1.0f, 1.0f, 0.22f);

        Color head_marker_col = has_theme_color("head_color", "CircularGauge")
                              ? get_theme_color("head_color", "CircularGauge")
                              : Color(0.98f, 0.78f, 0.15f, 1.0f); // Amber / Gold

        Color tail_marker_col = has_theme_color("tail_color", "CircularGauge")
                              ? get_theme_color("tail_color", "CircularGauge")
                              : Color(0.18f, 0.85f, 0.98f, 1.0f); // Electric Cyan

        float margin = has_theme_constant("margin", "CircularGauge")
                     ? static_cast<float>(get_theme_constant("margin", "CircularGauge"))
                     : 16.0f; // Leaves clearance for outward triangle markers

        float thickness = has_theme_constant("band_thickness", "CircularGauge")
                        ? static_cast<float>(get_theme_constant("band_thickness", "CircularGauge"))
                        : 28.0f;

        // --- 2. Calculate Radii & Center ---
        Vector2 size = get_size();
        Vector2 center = size / 2.0f;
        float max_radius = Math::min(size.x, size.y) / 2.0f;
        float outer_radius = Math::max(max_radius - margin, 0.1f);
        float inner_radius = Math::max(outer_radius - thickness, 10.0f);
        float total_thickness = outer_radius - inner_radius;

        float start_rad = Math::deg_to_rad(start_angle);
        float end_rad = Math::deg_to_rad(end_angle);

        int num_layers = MAX(1, layer_colors.size());
        float layer_gap = (num_layers > 1) ? 1.5f : 0.0f;
        float layer_thickness = (total_thickness - (layer_gap * (num_layers - 1))) / static_cast<float>(num_layers);
        if (layer_thickness < 2.0f) {
            layer_gap = 0.0f;
            layer_thickness = total_thickness / static_cast<float>(num_layers);
        }

        // --- 3. Draw Concentric Background Arcs ---
        for (int k = 0; k < num_layers; ++k) {
            float r_out = outer_radius - k * (layer_thickness + layer_gap);
            float r_in = r_out - layer_thickness;
            Color layer_bg = (layer_colors.size() > k) ? layer_colors[k].darkened(0.70f) : bg_color;
            layer_bg.a = 0.55f;
            _draw_gauge_segment(start_rad, end_rad, r_out, r_in, layer_bg);
        }

        // --- 4. Draw Active Range Payload Fills ---
        float angle_1 = _value_to_angle(value_1);
        float angle_2 = _value_to_angle(value_2);

        if (is_full) {
            for (int k = 0; k < num_layers; ++k) {
                float r_out = outer_radius - k * (layer_thickness + layer_gap);
                float r_in = r_out - layer_thickness;
                Color col = (layer_colors.size() > k) ? layer_colors[k] : fill_color;
                _draw_gauge_segment(start_rad, end_rad, r_out, r_in, col);
            }
        } else if (marker_count == 1) {
            for (int k = 0; k < num_layers; ++k) {
                float r_out = outer_radius - k * (layer_thickness + layer_gap);
                float r_in = r_out - layer_thickness;
                Color col = (layer_colors.size() > k) ? layer_colors[k] : fill_color;
                _draw_gauge_segment(start_rad, angle_1, r_out, r_in, col);
            }
        } else {
            if (is_wrapped) {
                for (int k = 0; k < num_layers; ++k) {
                    float r_out = outer_radius - k * (layer_thickness + layer_gap);
                    float r_in = r_out - layer_thickness;
                    Color col = (layer_colors.size() > k) ? layer_colors[k] : fill_color;
                    _draw_gauge_segment(angle_1, end_rad, r_out, r_in, col);
                    _draw_gauge_segment(start_rad, angle_2, r_out, r_in, col);
                }
            } else if (!Math::is_equal_approx(angle_1, angle_2)) {
                for (int k = 0; k < num_layers; ++k) {
                    float r_out = outer_radius - k * (layer_thickness + layer_gap);
                    float r_in = r_out - layer_thickness;
                    Color col = (layer_colors.size() > k) ? layer_colors[k] : fill_color;
                    _draw_gauge_segment(Math::min(angle_1, angle_2), Math::max(angle_1, angle_2), r_out, r_in, col);
                }
            }
        }

        // --- 5. Draw Hazard / Overflow Striping ---
        if (is_overflowing) {
            _draw_hazard_stripes(start_rad, end_rad, outer_radius, inner_radius);
        }

        // --- 6. Draw Slot Subdivision Ticks ---
        if (draw_slot_ticks && slot_count > 0) {
            float total_deg = end_angle - start_angle;
            int step_stride = 1;
            float arc_len_per_slot = (static_cast<float>(Math_TAU) * outer_radius) / static_cast<float>(slot_count);
            if (arc_len_per_slot < 3.0f) {
                step_stride = Math::ceil(3.0f / arc_len_per_slot);
            }

            for (int i = 0; i < slot_count; i += step_stride) {
                float ratio = static_cast<float>(i) / static_cast<float>(slot_count);
                float deg = start_angle + (total_deg * ratio);
                float rad = Math::deg_to_rad(deg);
                Vector2 dir(Math::cos(rad), Math::sin(rad));
                draw_line(center + dir * inner_radius, center + dir * outer_radius, tick_color, 1.0f);
            }
        }

        // --- 7. Draw Concentric Layer Borders ---
        Color border_col(0.10f, 0.10f, 0.12f, 0.90f);
        draw_arc(center, outer_radius, start_rad, end_rad, 64, border_col, 1.5f);
        draw_arc(center, inner_radius, start_rad, end_rad, 64, border_col, 1.5f);

        // --- 8. Accessible Head & Tail Markers ---
        if (draw_head_tail_markers && marker_count >= 2) {
            _draw_markers(center, outer_radius, inner_radius, head_marker_col, tail_marker_col);
        }
    }
}

void CircularGauge::_draw_markers(const Vector2& p_center, float p_outer_radius, float p_inner_radius, const Color& p_head_col, const Color& p_tail_col) {
    float angle_tail = _value_to_angle(value_1);
    float angle_head = _value_to_angle(value_2);
    float marker_sz = 12.0f;

    // A. Write Head (Outward Solid Triangle on Outer Radius)
    Vector2 dir_head(Math::cos(angle_head), Math::sin(angle_head));
    Vector2 perp_head(-dir_head.y, dir_head.x);
    Vector2 base_head = p_center + dir_head * p_outer_radius;
    Vector2 tip_head = base_head + dir_head * marker_sz;
    Vector2 corner1_head = base_head - perp_head * (marker_sz * 0.65f);
    Vector2 corner2_head = base_head + perp_head * (marker_sz * 0.65f);

    PackedVector2Array head_poly;
    head_poly.push_back(tip_head);
    head_poly.push_back(corner1_head);
    head_poly.push_back(corner2_head);

    PackedColorArray head_colors;
    head_colors.push_back(p_head_col);
    draw_polygon(head_poly, head_colors);

    // High-contrast accessibility border
    PackedVector2Array head_outline;
    head_outline.push_back(tip_head);
    head_outline.push_back(corner1_head);
    head_outline.push_back(corner2_head);
    head_outline.push_back(tip_head);
    draw_polyline(head_outline, Color(0.05f, 0.05f, 0.08f, 1.0f), 1.5f);

    // B. Read Tail (Inward Hollow Diamond on Inner Radius)
    Vector2 dir_tail(Math::cos(angle_tail), Math::sin(angle_tail));
    Vector2 perp_tail(-dir_tail.y, dir_tail.x);
    Vector2 diamond_outer = p_center + dir_tail * p_inner_radius;
    Vector2 diamond_inner = diamond_outer - dir_tail * marker_sz;
    Vector2 diamond_mid = diamond_outer - dir_tail * (marker_sz * 0.5f);
    Vector2 diamond_left = diamond_mid - perp_tail * (marker_sz * 0.5f);
    Vector2 diamond_right = diamond_mid + perp_tail * (marker_sz * 0.5f);

    PackedVector2Array diamond_poly;
    diamond_poly.push_back(diamond_outer);
    diamond_poly.push_back(diamond_right);
    diamond_poly.push_back(diamond_inner);
    diamond_poly.push_back(diamond_left);
    diamond_poly.push_back(diamond_outer);

    // Backing stroke for luminance contrast
    draw_polyline(diamond_poly, Color(0.05f, 0.05f, 0.08f, 1.0f), 3.5f);
    draw_polyline(diamond_poly, p_tail_col, 1.75f);
}

void CircularGauge::_draw_hazard_stripes(float p_start_rad, float p_end_rad, float p_outer_radius, float p_inner_radius) {
    Vector2 center = get_size() / 2.0f;
    float range = p_end_rad - p_start_rad;
    int stripe_count = 36;
    float step = range / static_cast<float>(stripe_count);

    Color hazard_stripe_col = (custom_overflow_color.a > 0.0f) ? custom_overflow_color : Color(0.95f, 0.20f, 0.20f, 0.75f);

    for (int i = 0; i < stripe_count; i += 2) {
        float theta1 = p_start_rad + (step * i);
        float theta2 = p_start_rad + (step * (i + 1));
        _draw_gauge_segment(theta1, theta2, p_outer_radius, p_inner_radius, hazard_stripe_col);
    }
}

float CircularGauge::_value_to_angle(float p_value) const {
    if (Math::is_equal_approx(min_value, max_value)) {
        return Math::deg_to_rad(start_angle);
    }
    
    float ratio = (p_value - min_value) / (max_value - min_value);
    ratio = CLAMP(ratio, 0.0f, 1.0f);
    
    float angle_deg = Math::lerp(start_angle, end_angle, ratio);
    return Math::deg_to_rad(angle_deg);
}

void CircularGauge::_draw_gauge_segment(float p_start_rad, float p_end_rad, float p_outer_radius, float p_inner_radius, const godot::Color& p_color) {
    if (Math::is_equal_approx(p_start_rad, p_end_rad)) return;

    Vector2 center = get_size() / 2.0f;
    int segments = 48; 
    
    PackedVector2Array polygon;
    PackedColorArray colors;
    colors.push_back(p_color); 

    float angle_range = p_end_rad - p_start_rad;
    float step = angle_range / static_cast<float>(segments);
    
    if (direction == DIRECTION_COUNTER_CLOCKWISE) {
        step = -step;
    }

    if (!draw_as_band) {
        polygon.push_back(center);
        for (int i = 0; i <= segments; ++i) {
            float theta = p_start_rad + (step * i);
            polygon.push_back(center + Vector2(Math::cos(theta), Math::sin(theta)) * p_outer_radius);
        }
    } else {
        PackedVector2Array outer_points;
        PackedVector2Array inner_points;
        float total_range = Math::deg_to_rad(end_angle) - Math::deg_to_rad(start_angle);

        for (int i = 0; i <= segments; ++i) {
            float theta = p_start_rad + (step * i);
            Vector2 dir(Math::cos(theta), Math::sin(theta));
            
            float current_inner_radius = p_inner_radius;

            if (width_curve.is_valid()) {
                float normalized_pos = (theta - Math::deg_to_rad(start_angle)) / total_range;
                normalized_pos = CLAMP(normalized_pos, 0.0f, 1.0f);
                float curve_multiplier = width_curve->sample_baked(normalized_pos);
                current_inner_radius = p_outer_radius - ((p_outer_radius - p_inner_radius) * curve_multiplier);
            }

            outer_points.push_back(center + dir * p_outer_radius);
            inner_points.push_back(center + dir * current_inner_radius);
        }

        polygon.append_array(outer_points);
        for (int i = inner_points.size() - 1; i >= 0; --i) {
            polygon.push_back(inner_points[i]);
        }
    }

    draw_polygon(polygon, colors);
}

Vector2 CircularGauge::_get_minimum_size() const {
    return Vector2(180.0f, 180.0f);
}

// Property Setters
void CircularGauge::set_direction(int p_direction) {
    direction = static_cast<GaugeDirection>(p_direction); queue_redraw();
}
int CircularGauge::get_direction() const { return direction; }

void CircularGauge::set_marker_count(int p_count) {
    marker_count = CLAMP(p_count, 1, 2); queue_redraw();
}
int CircularGauge::get_marker_count() const { return marker_count; }

void CircularGauge::set_start_angle(float p_angle) {
    start_angle = p_angle; queue_redraw();
}
float CircularGauge::get_start_angle() const { return start_angle; }

void CircularGauge::set_end_angle(float p_angle) {
    end_angle = p_angle; queue_redraw();
}
float CircularGauge::get_end_angle() const { return end_angle; }

void CircularGauge::set_min_value(float p_val) {
    min_value = p_val; queue_redraw();
}
float CircularGauge::get_min_value() const { return min_value; }

void CircularGauge::set_max_value(float p_val) {
    max_value = p_val; queue_redraw();
}
float CircularGauge::get_max_value() const { return max_value; }

void CircularGauge::set_value_1(float p_val) {
    value_1 = CLAMP(p_val, min_value, max_value); queue_redraw();
}
float CircularGauge::get_value_1() const { return value_1; }

void CircularGauge::set_value_2(float p_val) {
    value_2 = CLAMP(p_val, min_value, max_value); queue_redraw();
}
float CircularGauge::get_value_2() const { return value_2; }

void CircularGauge::set_draw_as_band(bool p_band) {
    draw_as_band = p_band; queue_redraw();
}
bool CircularGauge::get_draw_as_band() const { return draw_as_band; }

void CircularGauge::set_width_curve(const Ref<Curve>& p_curve) {
    if (width_curve == p_curve) return;
    if (width_curve.is_valid()) width_curve->disconnect("changed", Callable(this, "queue_redraw"));
    width_curve = p_curve;
    if (width_curve.is_valid()) width_curve->connect("changed", Callable(this, "queue_redraw"));
    queue_redraw();
}
Ref<Curve> CircularGauge::get_width_curve() const { return width_curve; }

void CircularGauge::set_is_wrapped(bool p_wrapped) {
    if (is_wrapped == p_wrapped) return;
    is_wrapped = p_wrapped; queue_redraw();
}
bool CircularGauge::get_is_wrapped() const { return is_wrapped; }

void CircularGauge::set_is_full(bool p_full) {
    if (is_full == p_full) return;
    is_full = p_full; queue_redraw();
}
bool CircularGauge::get_is_full() const { return is_full; }

void CircularGauge::set_is_overflowing(bool p_overflowing) {
    if (is_overflowing == p_overflowing) return;
    is_overflowing = p_overflowing; queue_redraw();
}
bool CircularGauge::get_is_overflowing() const { return is_overflowing; }

void CircularGauge::set_slot_count(int p_slots) {
    if (slot_count == p_slots) return;
    slot_count = Math::max(0, p_slots); queue_redraw();
}
int CircularGauge::get_slot_count() const { return slot_count; }

void CircularGauge::set_draw_slot_ticks(bool p_draw) {
    if (draw_slot_ticks == p_draw) return;
    draw_slot_ticks = p_draw; queue_redraw();
}
bool CircularGauge::get_draw_slot_ticks() const { return draw_slot_ticks; }

void CircularGauge::set_draw_head_tail_markers(bool p_draw) {
    if (draw_head_tail_markers == p_draw) return;
    draw_head_tail_markers = p_draw; queue_redraw();
}
bool CircularGauge::get_draw_head_tail_markers() const { return draw_head_tail_markers; }

void CircularGauge::set_layer_colors(const PackedColorArray& p_colors) {
    layer_colors = p_colors; queue_redraw();
}
PackedColorArray CircularGauge::get_layer_colors() const { return layer_colors; }

void CircularGauge::set_custom_bg_color(const Color& p_col) {
    custom_bg_color = p_col; queue_redraw();
}
Color CircularGauge::get_custom_bg_color() const { return custom_bg_color; }

void CircularGauge::set_custom_fill_color(const Color& p_col) {
    custom_fill_color = p_col; queue_redraw();
}
Color CircularGauge::get_custom_fill_color() const { return custom_fill_color; }

void CircularGauge::set_custom_overflow_color(const Color& p_col) {
    custom_overflow_color = p_col; queue_redraw();
}
Color CircularGauge::get_custom_overflow_color() const { return custom_overflow_color; }

} // namespace ideam::godot_ext