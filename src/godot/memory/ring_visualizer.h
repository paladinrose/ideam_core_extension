#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>

#include "memory_buffer_resource.h"
#include "../controls/circular_gauge.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class RingVisualizer : public godot::VBoxContainer {
    GDCLASS(RingVisualizer, godot::VBoxContainer)

    using DataType = ideam::core::DataType;

private:
    godot::Ref<MemoryBufferResource> active_buffer;

    // Emulation State
    int sim_head = 0;
    int sim_tail = 0;
    int sim_count = 0;
    bool is_overflowing = false;

    // UI Structure
    godot::HBoxContainer* toolbar = nullptr;
    godot::Button* btn_enqueue = nullptr;
    godot::Button* btn_dequeue = nullptr;
    godot::Button* btn_batch = nullptr;
    godot::Button* btn_reset = nullptr;
    godot::Label* telemetry_label = nullptr;
    godot::Label* hazard_badge = nullptr;

    godot::CenterContainer* gauge_container = nullptr;
    CircularGauge* gauge = nullptr;

    godot::HBoxContainer* legend_container = nullptr;

    void _clear_connections();
    void _update_gauge_and_ui();
    godot::Color _get_type_color(DataType p_type) const;
    godot::String _get_type_badge(DataType p_type) const;

protected:
    static void _bind_methods();

public:
    RingVisualizer();
    ~RingVisualizer();

    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    // Emulation Mutators
    void step_enqueue(int p_count = 1);
    void step_dequeue(int p_count = 1);
    void reset_simulation();

    int get_head_index() const;
    int get_tail_index() const;
    int get_element_count() const;
    bool get_is_overflowing() const;

    // Callbacks
    void _on_enqueue_pressed();
    void _on_dequeue_pressed();
    void _on_batch_pressed();
    void _on_reset_pressed();

    virtual godot::Vector2 _get_minimum_size() const override;
};

} // namespace ideam::godot_ext