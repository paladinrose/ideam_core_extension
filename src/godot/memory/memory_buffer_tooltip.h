#pragma once

#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class MemoryBufferTooltip : public godot::PanelContainer {
    GDCLASS(MemoryBufferTooltip, godot::PanelContainer)

private:
    godot::MarginContainer* root_margin = nullptr;
    godot::VBoxContainer* content_box = nullptr;

    void _configure_buffer(const godot::Dictionary& p_data);
    void _configure_managed(const godot::Dictionary& p_data);
    void _configure_transient(const godot::Dictionary& p_data);

    godot::String _format_bytes(int64_t p_bytes) const;
    godot::String _format_data_type(const godot::Variant& p_type_val) const;

    godot::Label* _create_meta_label(const godot::String& p_key, const godot::String& p_val, const godot::Color& p_val_color = godot::Color(0.85f, 0.85f, 0.85f, 1.0f));
    godot::Label* _create_badge(const godot::String& p_text, const godot::Color& p_color);
    godot::Label* _create_table_cell(const godot::String& p_text, bool p_is_header = false, const godot::Color& p_custom_color = godot::Color(1, 1, 1, 0));

protected:
    static void _bind_methods();

public:
    MemoryBufferTooltip();
    ~MemoryBufferTooltip() = default;

    void configure(const godot::Dictionary& p_data);
};

} // namespace ideam::godot_ext