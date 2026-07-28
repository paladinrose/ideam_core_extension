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
    godot::Ref<MemoryBufferResource> active_buffer;
    
    godot::Label* header_label = nullptr;
    godot::VBoxContainer* visualization_container = nullptr;

    void _clear_view();
    void _build_view();

    void _setup_ring_buffer_gauge(CircularGauge* p_gauge);
    void _setup_selection_bitset_grid(HeatmapGrid* p_grid);

protected:
    static void _bind_methods();

public:
    MemoryBufferView();
    ~MemoryBufferView();

    void open_buffer(godot::Ref<MemoryBufferResource> p_buffer);
    
    // Signal Handlers
    void _on_buffer_changed();
};

} // namespace ideam::godot_ext