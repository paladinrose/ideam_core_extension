#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/color.hpp>

namespace ideam::godot_ext { 

class BooleanHeatmapGrid : public godot::Control {
    GDCLASS(BooleanHeatmapGrid, godot::Control)

private:
    godot::PackedByteArray data;
    int columns = 16;
    bool square_cells = true;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
    // Ensures the control doesn't collapse to 0 in containers
    godot::Vector2 _get_minimum_size() const override;

public:
    BooleanHeatmapGrid();
    ~BooleanHeatmapGrid() = default;

    // Setters and Getters
    void set_data(const godot::PackedByteArray& p_data);
    godot::PackedByteArray get_data() const;

    void set_columns(int p_columns);
    int get_columns() const;

    void set_square_cells(bool p_square);
    bool get_square_cells() const;

    // Utility methods for direct manipulation
    void set_value(int p_index, bool p_value);
    bool get_value(int p_index) const;
    void clear_data();
    void resize_data(int p_size);
};

} // namespace ideam::godot_ext