#pragma once


#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>

#include "memory_buffer_resource.h"
#include "aos_visualizer.h"
#include "soa_visualizer.h"
#include "memory_page_grid.h"

#include "../controls/circular_gauge.h"
#include "../controls/heatmap_grid.h"

namespace ideam::godot_ext {

class MemoryBufferView : public godot::VBoxContainer {
    GDCLASS(MemoryBufferView, godot::VBoxContainer)

private:
    godot::TypedArray<MemoryBufferResource> active_buffers;
    
    godot::Label* header_label = nullptr;
    godot::VBoxContainer* visualization_container = nullptr;

    void _clear_view();
    void _build_view();

    void _setup_ring_buffer_gauge(CircularGauge* p_gauge, godot::Ref<MemoryBufferResource> p_buffer);
    void _setup_selection_bitset_grid(HeatmapGrid* p_grid, godot::Ref<MemoryBufferResource> p_buffer);

protected:
    static void _bind_methods();

public:
    MemoryBufferView();
    ~MemoryBufferView();

    void open_buffers(const godot::TypedArray<MemoryBufferResource>& p_buffers);
    
    // Signal Handlers
    void _on_buffer_changed();
};

} // namespace ideam::godot_ext