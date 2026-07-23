#include "circular_gauge.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

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

    ClassDB::bind_method(D_METHOD("set_is_overflowing", "overflowing"), &CircularGauge::set_is_overflowing);
    ClassDB::bind_method(D_METHOD("get_is_overflowing"), &CircularGauge::get_is_overflowing);

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
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_overflowing"), "set_is_overflowing", "get_is_overflowing");
}

CircularGauge::CircularGauge() {
    set_clip_contents(true);
}

void CircularGauge::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    }
    else if (p_what == NOTIFICATION_DRAW) {
        // --- 1. Fetch Theme Data & Handle Defaults ---
        Color bg_color = has_theme_color("background_color", "CircularGauge") 
                       ? get_theme_color("background_color", "CircularGauge") 
                       : Color(0.2f, 0.2f, 0.2f, 0.5f);
                       
        Color fill_color = has_theme_color("fill_color", "CircularGauge") 
                         ? get_theme_color("fill_color", "CircularGauge") 
                         : Color(0.2f, 0.8f, 0.4f, 1.0f);

        Color overflow_color = has_theme_color("overflow_color", "CircularGauge") 
                         ? get_theme_color("overflow_color", "CircularGauge") 
                         : Color(0.9f, 0.2f, 0.2f, 1.0f); // Defaults to a bright red
                         
        float margin = has_theme_constant("margin", "CircularGauge") 
                     ? static_cast<float>(get_theme_constant("margin", "CircularGauge")) 
                     : 0.0f;
                     
        float thickness = has_theme_constant("band_thickness", "CircularGauge") 
                        ? static_cast<float>(get_theme_constant("band_thickness", "CircularGauge")) 
                        : 10.0f;

        // --- 2. Dynamically Calculate Radii based on Control Size ---
        Vector2 size = get_size();
        float max_radius = MIN(size.x, size.y) / 2.0f;
        float outer_radius = MAX(max_radius - margin, 0.1f);
        float inner_radius = MAX(outer_radius - thickness, 0.0f);

        // --- 3. Compute Base Angles ---
        float start_rad = Math::deg_to_rad(start_angle);
        float end_rad = Math::deg_to_rad(end_angle);
        
        // --- 4. Draw Background ---
        _draw_gauge_segment(start_rad, end_rad, outer_radius, inner_radius, bg_color);

        // --- 5. Calculate Fill Angles & Draw Active Gauge ---
        float angle_1 = _value_to_angle(value_1);
        
        if (marker_count == 1) {
            _draw_gauge_segment(start_rad, angle_1, outer_radius, inner_radius, fill_color);
        } else {
            float angle_2 = _value_to_angle(value_2);
            
            // Draw Main Active Data Fill
            if (is_wrapped) {
                // Buffer spans across the visual seam
                _draw_gauge_segment(angle_1, end_rad, outer_radius, inner_radius, fill_color);
                _draw_gauge_segment(start_rad, angle_2, outer_radius, inner_radius, fill_color);
            } else {
                // Standard continuous segment
                _draw_gauge_segment(MIN(angle_1, angle_2), MAX(angle_1, angle_2), outer_radius, inner_radius, fill_color);
            }

            // --- 6. Draw Overflow Overlay ---
            if (is_overflowing) {
                // If angle_1 > angle_2, the overwrite spans across the start/end bounds
                if (angle_1 > angle_2) {
                    _draw_gauge_segment(angle_1, end_rad, outer_radius, inner_radius, overflow_color);
                    _draw_gauge_segment(start_rad, angle_2, outer_radius, inner_radius, overflow_color);
                } else {
                    _draw_gauge_segment(angle_1, angle_2, outer_radius, inner_radius, overflow_color);
                }
            }
        }
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
    int segments = 64; 
    
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

// --- Standard Setters (Triggering Redraws) ---

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
    
    if (width_curve.is_valid()) {
        width_curve->disconnect("changed", Callable(this, "queue_redraw"));
    }
    
    width_curve = p_curve;
    
    if (width_curve.is_valid()) {
        width_curve->connect("changed", Callable(this, "queue_redraw"));
    }
    queue_redraw();
}
Ref<Curve> CircularGauge::get_width_curve() const { return width_curve; }

void CircularGauge::set_is_wrapped(bool p_wrapped) {
    is_wrapped = p_wrapped; queue_redraw();
}
bool CircularGauge::get_is_wrapped() const { return is_wrapped; }

void CircularGauge::set_is_overflowing(bool p_overflowing) {
    is_overflowing = p_overflowing; queue_redraw();
}
bool CircularGauge::get_is_overflowing() const { return is_overflowing; }

} // namespace ideam::godot_ext