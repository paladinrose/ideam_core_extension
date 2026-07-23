#pragma once

#include <godot_cpp/classes/control.hpp>
#include "memory_buffer_resource.h"

namespace ideam::godot_ext {

class MemoryPageGrid : public godot::Control {
    GDCLASS(MemoryPageGrid, godot::Control)

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    
    int page_size = 4096; // Standard 4KB OS Page
    int target_columns = 16;
    bool enforce_square_cells = true;

    void _clear_connections();

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
    godot::Vector2 _get_minimum_size() const override;

public:
    MemoryPageGrid();
    ~MemoryPageGrid();

    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    void set_page_size(int p_size);
    int get_page_size() const;

    void set_target_columns(int p_columns);
    int get_target_columns() const;

    void set_enforce_square_cells(bool p_enforce);
    bool get_enforce_square_cells() const;
};

} // namespace ideam::godot_ext