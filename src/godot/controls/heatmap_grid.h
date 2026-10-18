#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext { 

class HeatmapGrid : public godot::Control {
    GDCLASS(HeatmapGrid, godot::Control)

public:
    enum CellState {
        STATE_UNSELECTED = 0,
        STATE_SELECTED   = 1,
        STATE_HOVERED    = 2,
        STATE_PENDING    = 3
    };

private:
    godot::PackedByteArray data;
    godot::Ref<godot::Gradient> color_gradient;
    int columns = 64;
    bool square_cells = true;

    // Interactive State Tracking
    int hovered_index = -1;
    int selected_index = -1;

    // Helper Geometry & Coloring
    godot::Color _get_color_for_value(uint8_t p_value) const;
    int _get_cell_index_at_pos(const godot::Vector2& p_pos) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    

public:
    HeatmapGrid();
    ~HeatmapGrid() = default;

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    // Setters and Getters
    void set_data(const godot::PackedByteArray& p_data);
    godot::PackedByteArray get_data() const;

    void set_gradient(const godot::Ref<godot::Gradient>& p_gradient);
    godot::Ref<godot::Gradient> get_gradient() const;

    void set_columns(int p_columns);
    int get_columns() const;

    void set_square_cells(bool p_square);
    bool get_square_cells() const;

    void set_selected_index(int p_index);
    int get_selected_index() const;

    void set_hovered_index(int p_index);
    int get_hovered_index() const;

    // Data Manipulation
    void set_value(int p_index, uint8_t p_value);
    uint8_t get_value(int p_index) const;
    void clear_data();
    void resize_data(int p_size);

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::HeatmapGrid::CellState);