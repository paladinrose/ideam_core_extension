#pragma once

#include "memory_buffer_tooltip.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class MemoryRibbonButton : public godot::Button {
    GDCLASS(MemoryRibbonButton, godot::Button)

private:
    int block_type = 0;
    int index = -1;

    bool is_selected = false;
    bool is_dimmed = false;

    void _update_visual_state();

protected:
    static void _bind_methods();

public:
    MemoryRibbonButton();
    ~MemoryRibbonButton() = default;

    // Godot Control Overrides
    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    virtual godot::Object* _make_custom_tooltip(const godot::String& p_for_text) const override;

    // Block Identity
    void set_block_type(int p_type);
    int get_block_type() const { return block_type; }

    void set_index(int p_index);
    int get_index() const { return index; }

    // Visual State Management
    void set_selected(bool p_selected);
    bool get_selected() const { return is_selected; }

    void set_dimmed(bool p_dimmed);
    bool get_dimmed() const { return is_dimmed; }
};

} // namespace ideam::godot_ext