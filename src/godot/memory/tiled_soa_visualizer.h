#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class TiledSoAVisualizer : public godot::Control {
    GDCLASS(TiledSoAVisualizer, godot::Control)

    using DataType = ideam::core::DataType;

public:
    struct SubBlockInfo {
        godot::String name;
        DataType data_type = DataType::FLOAT32;
        int col_size = 4;
        int alignment = 4;
        size_t intra_offset = 0;
        size_t byte_size = 0;
        bool is_padding = false;
        godot::Rect2 rect;
    };

    struct TileLayout {
        int tile_index = 0;
        int element_start = 0;
        int element_count = 0;
        size_t tile_offset_bytes = 0;
        size_t raw_bytes = 0;
        size_t stride_bytes = 0;
        size_t padding_bytes = 0;
        godot::Rect2 macro_rect;
    };

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    godot::Vector<TileLayout> cached_tiles;
    godot::Vector<SubBlockInfo> cached_sub_blocks;

    size_t single_tile_stride = 0;
    size_t single_tile_raw = 0;
    size_t single_tile_padding = 0;

    int focused_tile_index = 0;
    int hovered_tile_index = -1;
    int hovered_sub_block_index = -1;
    bool show_locality_comparison = true;

    void _clear_connections();
    void _recalculate_layout();
    void _draw_crosshatch(const godot::Rect2& p_rect, const godot::Color& p_line_color, float p_spacing = 6.0f);

    godot::Color _get_type_color(DataType p_type) const;
    godot::String _get_type_badge(DataType p_type) const;
    godot::String _get_type_name(DataType p_type) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
public:
    TiledSoAVisualizer();
    ~TiledSoAVisualizer();

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    void set_focused_tile_index(int p_index);
    int get_focused_tile_index() const;

    void set_show_locality_comparison(bool p_show);
    bool get_show_locality_comparison() const;

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext