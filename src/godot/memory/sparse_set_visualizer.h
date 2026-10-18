#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class SparseSetVisualizer : public godot::Control {
    GDCLASS(SparseSetVisualizer, godot::Control)

    using DataType = ideam::core::DataType;

public:
    struct DenseColumnInfo {
        godot::String name;
        DataType data_type = DataType::FLOAT32;
        int col_size = 4;
        int alignment = 4;
        size_t stream_offset = 0;
        size_t stream_bytes = 0;
        godot::Rect2 ribbon_rect;
    };

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    godot::Vector<DenseColumnInfo> cached_columns;

    // Simulation / Emulation Mapping
    int simulated_dense_count = 12;
    godot::PackedInt64Array dense_to_entity_map;

    // Track Geometry
    godot::Rect2 sparse_tier_rect;
    godot::Rect2 dense_tier_rect;
    float trace_channel_y = 0.0f;
    float trace_channel_height = 40.0f;

    // Interactive Hover Tracking
    int hovered_tier = -1; // 0 = Sparse, 1 = Dense
    int64_t hovered_entity_id = -1;
    int hovered_dense_index = -1;

    void _clear_connections();
    void _recalculate_layout();
    void _ensure_simulation_mapping();

    int _find_dense_for_entity(int64_t p_entity_id) const;
    int64_t _get_entity_for_dense(int p_dense_idx) const;

    godot::Color _get_type_color(DataType p_type) const;
    godot::String _get_type_badge(DataType p_type) const;
    godot::String _get_type_name(DataType p_type) const;
    void _draw_bezier_link(const godot::Vector2& p_start, const godot::Vector2& p_end, const godot::Color& p_color, float p_width = 2.0f);

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
public:
    SparseSetVisualizer();
    ~SparseSetVisualizer();

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    
    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    // Simulation Mapping Accessors
    void set_simulated_dense_count(int p_count);
    int get_simulated_dense_count() const;

    void set_dense_to_entity_map(const godot::PackedInt64Array& p_map);
    godot::PackedInt64Array get_dense_to_entity_map() const;

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext