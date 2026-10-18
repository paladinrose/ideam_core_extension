#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

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
    int marker_count = 2;

    // Angle and Value mapping
    float start_angle = 0.0f; // In degrees
    float end_angle = 360.0f; // In degrees
    float min_value = 0.0f;   
    float max_value = 100.0f; 

    // Marker values
    float value_1 = 0.0f; // Tail / Read Index
    float value_2 = 0.0f; // Head / Write Index

    // Drawing properties
    bool draw_as_band = true;
    godot::Ref<godot::Curve> width_curve;
    
    // Ring Buffer Specifics
    bool is_wrapped = false;
    bool is_full = false;
    bool is_overflowing = false;

    // Slot Subdivision & Concentric Tuple Layers
    int slot_count = 0;
    bool draw_slot_ticks = true;
    bool draw_head_tail_markers = true;
    godot::PackedColorArray layer_colors;

    // Custom Color Overrides (theme fallbacks applied when alpha == 0)
    godot::Color custom_bg_color = godot::Color(0, 0, 0, 0);
    godot::Color custom_fill_color = godot::Color(0, 0, 0, 0);
    godot::Color custom_overflow_color = godot::Color(0, 0, 0, 0);

    // Helpers
    void _draw_gauge_segment(float p_start_rad, float p_end_rad, float p_outer_radius, float p_inner_radius, const godot::Color& p_color);
    void _draw_hazard_stripes(float p_start_rad, float p_end_rad, float p_outer_radius, float p_inner_radius);
    void _draw_markers(const godot::Vector2& p_center, float p_outer_radius, float p_inner_radius, const godot::Color& p_head_col, const godot::Color& p_tail_col);
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

    void set_is_full(bool p_full);
    bool get_is_full() const;

    void set_is_overflowing(bool p_overflowing);
    bool get_is_overflowing() const;

    void set_slot_count(int p_slots);
    int get_slot_count() const;

    void set_draw_slot_ticks(bool p_draw);
    bool get_draw_slot_ticks() const;

    void set_draw_head_tail_markers(bool p_draw);
    bool get_draw_head_tail_markers() const;

    void set_layer_colors(const godot::PackedColorArray& p_colors);
    godot::PackedColorArray get_layer_colors() const;

    void set_custom_bg_color(const godot::Color& p_col);
    godot::Color get_custom_bg_color() const;

    void set_custom_fill_color(const godot::Color& p_col);
    godot::Color get_custom_fill_color() const;

    void set_custom_overflow_color(const godot::Color& p_col);
    godot::Color get_custom_overflow_color() const;

    virtual godot::Vector2 _get_minimum_size() const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CircularGauge::GaugeDirection);