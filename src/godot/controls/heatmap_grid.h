#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/color.hpp>

namespace ideam::godot_ext { 

class HeatmapGrid : public godot::Control {
    GDCLASS(HeatmapGrid, godot::Control)

private:
    godot::PackedByteArray data;
    godot::Ref<godot::Gradient> color_gradient;
    int columns = 16;
    bool square_cells = true;

    // Helper to determine the color of a specific byte value
    godot::Color _get_color_for_value(uint8_t p_value) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    HeatmapGrid();
    ~HeatmapGrid() = default;

    // Setters and Getters
    void set_data(const godot::PackedByteArray& p_data);
    godot::PackedByteArray get_data() const;

    void set_gradient(const godot::Ref<godot::Gradient>& p_gradient);
    godot::Ref<godot::Gradient> get_gradient() const;

    void set_columns(int p_columns);
    int get_columns() const;

    void set_square_cells(bool p_square);
    bool get_square_cells() const;

    // Utility methods for direct manipulation
    void set_value(int p_index, uint8_t p_value);
    uint8_t get_value(int p_index) const;
    void clear_data();
    void resize_data(int p_size);

    // Ensures the control doesn't collapse to 0 in containers
    godot::Vector2 _get_minimum_size() const override;
};

} // namespace ideam::godot_ext