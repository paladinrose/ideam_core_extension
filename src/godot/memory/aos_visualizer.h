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

class AoSVisualizer : public godot::Control {
    GDCLASS(AoSVisualizer, godot::Control)

    using DataType = ideam::core::DataType;
    using BufferAlignmentMode = ideam::core::BufferAlignmentMode;

public:
    struct RenderBlock {
        godot::String name;
        int size_bytes = 0;
        int offset_bytes = 0;
        int alignment = 1;
        DataType data_type = DataType::BYTE;
        bool is_padding = false;
        bool is_trailing_padding = false;
        godot::Rect2 rect;
    };

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    godot::Vector<RenderBlock> cached_blocks;

    int total_stride_bytes = 0;
    int max_struct_alignment = 1;
    int total_padding_bytes = 0;
    int hovered_block_index = -1;

    void _clear_connections();
    void _recalculate_layout();
    void _draw_crosshatch(const godot::Rect2& p_rect, const godot::Color& p_line_color, float p_spacing = 8.0f);

    godot::Color _get_type_color(DataType p_type) const;
    godot::String _get_type_badge(DataType p_type) const;
    godot::String _get_type_name(DataType p_type) const;
    godot::String _get_alignment_mode_name(BufferAlignmentMode p_mode) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    AoSVisualizer();
    ~AoSVisualizer();

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    
    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext