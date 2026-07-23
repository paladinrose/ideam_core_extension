#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/variant/color.hpp>

namespace ideam::godot_ext { 

class CircularGauge : public godot::Control {
    GDCLASS(CircularGauge, godot::Control)

public:
    enum GaugeDirection {
        DIRECTION_CLOCKWISE = 0,
        DIRECTION_COUNTER_CLOCKWISE = 1
    };

private:
    GaugeDirection direction = DIRECTION_CLOCKWISE;
    int marker_count = 1; // 1 or 2 markers

    // Angle and Value mapping
    float start_angle = 0.0f; // In degrees
    float end_angle = 360.0f; // In degrees
    float min_value = 0.0f;   
    float max_value = 100.0f; 

    // Marker values
    float value_1 = 0.0f; // Typically Head or Tail
    float value_2 = 100.0f; // Only used if marker_count == 2

    // Drawing properties
    bool draw_as_band = false;
    godot::Ref<godot::Curve> width_curve;
    
    // Ring Buffer Specifics
    bool is_wrapped = false;
    bool is_overflowing = false;

    // Internal helper for generating the curved polygon
    void _draw_gauge_segment(float p_start_rad, float p_end_rad, float p_outer_radius, float p_inner_radius, const godot::Color& p_color);
    float _value_to_angle(float p_value) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    CircularGauge();
    ~CircularGauge() = default;

    // Setters and Getters
    void set_direction(int p_direction);
    int get_direction() const;

    void set_marker_count(int p_count);
    int get_marker_count() const;

    void set_start_angle(float p_angle);
    float get_start_angle() const;

    void set_end_angle(float p_angle);
    float get_end_angle() const;

    void set_min_value(float p_val);
    float get_min_value() const;

    void set_max_value(float p_val);
    float get_max_value() const;

    void set_value_1(float p_val);
    float get_value_1() const;

    void set_value_2(float p_val);
    float get_value_2() const;

    void set_draw_as_band(bool p_band);
    bool get_draw_as_band() const;

    void set_width_curve(const godot::Ref<godot::Curve>& p_curve);
    godot::Ref<godot::Curve> get_width_curve() const;

    void set_is_wrapped(bool p_wrapped);
    bool get_is_wrapped() const;

    void set_is_overflowing(bool p_overflowing);
    bool get_is_overflowing() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CircularGauge::GaugeDirection);