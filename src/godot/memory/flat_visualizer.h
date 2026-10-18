#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/color.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class FlatVisualizer : public godot::Control {
    GDCLASS(FlatVisualizer, godot::Control)

    using DataType = ideam::core::DataType;

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    int hovered_element = -1;

    void _clear_connections();
    godot::Color _get_data_type_color(DataType p_type) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
public:
    FlatVisualizer();
    ~FlatVisualizer();

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    
    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext