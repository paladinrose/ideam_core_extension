#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"
#include "../../core/memory/memory_buffer_pod.h"

namespace ideam::godot_ext {

class MemoryBufferColumnPanel : public godot::VBoxContainer {
    GDCLASS(MemoryBufferColumnPanel, godot::VBoxContainer)

    using DataType = ideam::core::DataType;

public:
    struct LeafTypeInfo {
        const char* name;
        DataType type;
    };

    static const LeafTypeInfo SUPPORTED_LEAF_TYPES[];
    static const size_t SUPPORTED_LEAF_TYPE_COUNT;

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    bool is_updating_ui = false;

    // Header UI
    godot::HBoxContainer* header_bar = nullptr;
    godot::Label* column_count_label = nullptr;
    godot::Button* add_column_btn = nullptr;
    godot::Button* clear_columns_btn = nullptr;

    // Scrollable Rows
    godot::ScrollContainer* scroll_container = nullptr;
    godot::VBoxContainer* rows_container = nullptr;

    // Telemetry Footer
    godot::HBoxContainer* footer_bar = nullptr;
    godot::Label* footprint_label = nullptr;

    void _clear_connections();
    void _rebuild_ui();
    void _update_footer();
    static int _find_type_index(DataType p_type);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    MemoryBufferColumnPanel();
    ~MemoryBufferColumnPanel();

    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);

    // Callbacks
    void _on_buffer_changed();
    void _on_add_column_pressed();
    void _on_clear_columns_pressed();
    void _on_column_name_changed(const godot::String& p_name, int p_index);
    void _on_column_type_selected(int p_type_index, int p_col_index);
    void _on_column_custom_size_changed(double p_size, int p_col_index);
    void _on_column_move_up(int p_index);
    void _on_column_move_down(int p_index);
    void _on_column_delete(int p_index);
};

} // namespace ideam::godot_ext