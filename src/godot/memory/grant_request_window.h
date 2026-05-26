#pragma once

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class GrantRequestWindow : public godot::AcceptDialog {
    GDCLASS(GrantRequestWindow, godot::AcceptDialog)

private:
    // --- UI Layout Controls ---
    godot::VBoxContainer* main_layout = nullptr;
    godot::ScrollContainer* parts_scroll = nullptr;
    godot::VBoxContainer* parts_container = nullptr;
    godot::Button* add_part_btn = nullptr;

    // --- Data Cache ---
    godot::TypedArray<godot::StringName> available_buffer_names;
    godot::TypedArray<godot::OptionButton> active_option_buttons;
    godot::StringName requesting_node_name;

protected:
    static void _bind_methods();
    
    // --- Internal Event Handlers ---
    void _on_add_part_pressed();
    void _on_remove_part_pressed(godot::Node* p_row_container);
    void _on_confirmed(); // Tied cleanly to AcceptDialog's built-in confirmed signal

public:
    GrantRequestWindow();
    virtual ~GrantRequestWindow() override = default;

    /**
     * @brief Prepares and opens the dialog modal.
     */
    void clear_and_popup(const godot::StringName& p_node_name, const godot::TypedArray<godot::StringName>& p_buffer_names);
};

} // namespace ideam::godot_ext