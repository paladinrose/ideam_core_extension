#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class SoAVisualizer : public godot::Control {
    GDCLASS(SoAVisualizer, godot::Control)

    using DataType = ideam::core::DataType;

public:
    struct RibbonMetadata {
        godot::String name;
        DataType data_type = DataType::FLOAT32;
        int col_size = 4;
        int alignment = 4;
        size_t stream_offset_bytes = 0;
        size_t total_stream_bytes = 0;
        godot::Rect2 rect;
    };

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    godot::Vector<RibbonMetadata> cached_ribbons;

    // Compaction & Lifecycle Tracking
    bool needs_compaction = false;
    int active_element_count = -1; // -1 defaults to max_elements
    int dead_element_count = 0;
    godot::PackedInt32Array tombstone_indices;

    // Interactive Hover Tracking
    int hovered_element_index = -1;
    int hovered_ribbon_index = -1;

    void _clear_connections();
    void _recalculate_layout();
    void _draw_crosshatch(const godot::Rect2& p_rect, const godot::Color& p_line_color, float p_spacing = 6.0f);

    godot::Color _get_type_color(DataType p_type) const;
    godot::String _get_type_badge(DataType p_type) const;
    godot::String _get_type_name(DataType p_type) const;
    bool _is_element_dead(int p_index, int p_effective_active) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
public:
    SoAVisualizer();
    ~SoAVisualizer();

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    
    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    // Compaction & Simulation Mutators
    void set_needs_compaction(bool p_needed);
    bool get_needs_compaction() const;

    void set_active_element_count(int p_count);
    int get_active_element_count() const;

    void set_dead_element_count(int p_count);
    int get_dead_element_count() const;

    void set_tombstone_indices(const godot::PackedInt32Array& p_indices);
    godot::PackedInt32Array get_tombstone_indices() const;

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext