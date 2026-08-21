#include "memory_buffer_view.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>


using namespace godot;

namespace ideam::godot_ext {

void MemoryBufferView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_buffers", "buffers"), &MemoryBufferView::open_buffers);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &MemoryBufferView::_on_buffer_changed);
}

MemoryBufferView::MemoryBufferView() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    header_label = memnew(Label);
    header_label->set_theme_type_variation("HeaderLarge");
    header_label->set_text("No Buffer Selected");
    add_child(header_label);

    visualization_container = memnew(VBoxContainer);
    visualization_container->set_h_size_flags(SIZE_EXPAND_FILL);
    visualization_container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(visualization_container);
}

MemoryBufferView::~MemoryBufferView() {}

void MemoryBufferView::open_buffers(const TypedArray<MemoryBufferResource>& p_buffers) {
    // Disconnect old signals
    for (int i = 0; i < active_buffers.size(); ++i) {
        Ref<MemoryBufferResource> buf = active_buffers[i];
        if (buf.is_valid() && buf->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
            buf->disconnect("changed", Callable(this, "_on_buffer_changed"));
        }
    }

    active_buffers = p_buffers;

    // Connect new signals
    for (int i = 0; i < active_buffers.size(); ++i) {
        Ref<MemoryBufferResource> buf = active_buffers[i];
        if (buf.is_valid()) {
            buf->connect("changed", Callable(this, "_on_buffer_changed"));
        }
    }

    _build_view();
}

void MemoryBufferView::_on_buffer_changed() {
    _build_view();
}

void MemoryBufferView::_clear_view() {
    for (int i = visualization_container->get_child_count() - 1; i >= 0; --i) {
        Node* child = visualization_container->get_child(i);
        visualization_container->remove_child(child);
        child->queue_free();
    }
}

void MemoryBufferView::_build_view() {
    _clear_view();

    if (active_buffers.is_empty()) {
        header_label->set_text("No Buffer Selected");
        return;
    }

    // MULTI-BUFFER INTERCEPT
    if (active_buffers.size() > 1) {
        header_label->set_text(String("Multiple Buffers Selected (") + String::num_int64(active_buffers.size()) + ")");
        
        Label* multi_unsupported_lbl = memnew(Label);
        multi_unsupported_lbl->set_text("Multi-buffer Editing isn't available! Please select a single Memory Buffer to edit.");
        multi_unsupported_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        multi_unsupported_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        visualization_container->add_child(multi_unsupported_lbl);
        return;
    }

    // SINGLE BUFFER VISUALIZATION
    Ref<MemoryBufferResource> active_buffer = active_buffers[0];
    if (!active_buffer.is_valid()) return;

    String b_name = active_buffer->get_buffer_name();
    header_label->set_text(b_name.is_empty() ? "Unnamed Buffer" : b_name);

    int layout = active_buffer->get_layout_type();
    switch (layout) {
        case MemoryBufferResource::LAYOUT_AOS: {
            AoSVisualizer* aos_vis = memnew(AoSVisualizer);
            aos_vis->setup_from_resource(active_buffer); 
            visualization_container->add_child(aos_vis);
            break;
        }
        case MemoryBufferResource::LAYOUT_SOA:
        case MemoryBufferResource::LAYOUT_TILED_SOA: {
            SoAVisualizer* soa_vis = memnew(SoAVisualizer);
            soa_vis->setup_from_resource(active_buffer); 
            visualization_container->add_child(soa_vis);
            break;
        }
        case MemoryBufferResource::LAYOUT_RING: {
            CircularGauge* ring_gauge = memnew(CircularGauge);
            _setup_ring_buffer_gauge(ring_gauge, active_buffer);
            visualization_container->add_child(ring_gauge);
            break;
        }
        case MemoryBufferResource::LAYOUT_PAGED: {
            MemoryPageGrid* page_vis = memnew(MemoryPageGrid);
            page_vis->setup_from_resource(active_buffer); 
            visualization_container->add_child(page_vis);
            break;
        }
        default: {
            Label* unsupported_lbl = memnew(Label);
            unsupported_lbl->set_text("Visualization not yet supported for this Layout Type.");
            visualization_container->add_child(unsupported_lbl);
            break;
        }
    }

    if (active_buffer->get_enable_shadowing()) {
        HeatmapGrid* bitset_grid = memnew(HeatmapGrid);
        _setup_selection_bitset_grid(bitset_grid, active_buffer); 
        visualization_container->add_child(bitset_grid);
    }
}

void MemoryBufferView::_setup_ring_buffer_gauge(CircularGauge* p_gauge, godot::Ref<MemoryBufferResource> p_buffer) {
    if (!p_buffer.is_valid() || !p_gauge) return;

    // Expand to fill the container space
    p_gauge->set_h_size_flags(SIZE_EXPAND_FILL);
    p_gauge->set_v_size_flags(SIZE_EXPAND_FILL);

    // Map resource capacity to the gauge's value range
    p_gauge->set_min_value(0.0f);
    p_gauge->set_max_value(static_cast<float>(p_buffer->get_max_elements()));

    // Configure as a full circular band suitable for ring buffer visualization
    p_gauge->set_start_angle(0.0f);
    p_gauge->set_end_angle(360.0f);
    p_gauge->set_draw_as_band(true);

    // NEW: Set up two markers for Head and Tail
    p_gauge->set_marker_count(2);
    p_gauge->set_value_1(0.0f); // e.g., Tail / Read Index
    p_gauge->set_value_2(0.0f); // e.g., Head / Write Index
    
    // NEW: Initialize ring buffer specific states
    p_gauge->set_is_wrapped(false);
    p_gauge->set_is_overflowing(false);
}

void MemoryBufferView::_setup_selection_bitset_grid(HeatmapGrid* p_grid, godot::Ref<MemoryBufferResource> p_buffer) {
    if (!p_buffer.is_valid() || !p_grid) return;

    p_grid->set_h_size_flags(SIZE_EXPAND_FILL);
    p_grid->set_v_size_flags(SIZE_EXPAND_FILL); 

    int max_elements = p_buffer->get_max_elements();
    
    // Allocate the underlying PackedByteArray to match the buffer capacity
    p_grid->resize_data(max_elements);
    
    // Defaulting to 64 columns visually represents standard 64-bit bitmasks well
    p_grid->set_columns(64);
    
    // Ensure the cells stay perfectly square for that classic bitset/heatmap look
    p_grid->set_square_cells(true);

    // Note: If you want to visually distinguish between DENSE and SPARSE selection modes
    // (e.g., Sparse might just show active pages rather than individual elements),
    // you can branch that logic here using p_buffer->get_selection_mode().
}

} // namespace ideam::godot_ext