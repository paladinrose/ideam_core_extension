#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/color.hpp>

namespace ideam::godot_ext {

class MemoryBlockButton : public godot::Button {
    GDCLASS(MemoryBlockButton, godot::Button)

private:
    int buffer_id = -1;
    bool is_selected = false;
    bool is_dimmed = false;

    // Optional slot container for embedded BlockGrantButtons
    godot::HBoxContainer* grant_icon_container = nullptr;

    void _update_visual_state();

protected:
    static void _bind_methods();

    // Godot Control Virtual Overrides
    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    virtual godot::Variant _get_drag_data(const godot::Vector2& p_at_position) override;
    virtual godot::Object* _make_custom_tooltip(const godot::String& p_for_text) const override;

public:
    MemoryBlockButton();
    ~MemoryBlockButton() = default;

    // Buffer Binding & State
    void set_buffer_id(int p_id);
    int get_buffer_id() const { return buffer_id; }

    void set_selected(bool p_selected);
    bool get_selected() const { return is_selected; }

    void set_dimmed(bool p_dimmed);
    bool get_dimmed() const { return is_dimmed; }

    // Grant Icon Attachment
    godot::HBoxContainer* get_grant_icon_container() const { return grant_icon_container; }
};

} // namespace ideam::godot_ext