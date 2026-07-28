#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>

#include "managed_buffer_resource.h"

namespace ideam::godot_ext {

class ManagedBufferView : public godot::VBoxContainer {
    GDCLASS(ManagedBufferView, godot::VBoxContainer)

private:
    godot::Ref<ManagedBufferResource> active_resource;
    
    godot::Label* header_label = nullptr;
    godot::GridContainer* metrics_grid = nullptr;

    // Value Labels
    godot::Label* val_consumer = nullptr;
    godot::Label* val_purpose = nullptr;
    godot::Label* val_layout = nullptr;
    godot::Label* val_footprint = nullptr;
    godot::Label* val_alignment = nullptr;

    void _setup_ui();
    void _clear_view();
    void _build_view();
    
    godot::String _get_layout_string(int p_layout) const;

protected:
    static void _bind_methods();

public:
    ManagedBufferView();
    ~ManagedBufferView();

    void open_resource(godot::Ref<ManagedBufferResource> p_resource);
    
    // Signal Handlers
    void _on_resource_changed();
};

} // namespace ideam::godot_ext