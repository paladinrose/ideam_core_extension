#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include "memory_buffer_resource.h"

namespace ideam::godot_ext {

class SoAVisualizer : public godot::Control {
    GDCLASS(SoAVisualizer, godot::Control)

private:
    godot::Ref<MemoryBufferResource> active_buffer;

    void _clear_connections();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SoAVisualizer();
    ~SoAVisualizer();

    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    godot::Vector2 _get_minimum_size() const override;
};

} // namespace ideam::godot_ext