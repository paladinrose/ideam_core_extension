#include "grant_request_window.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

using namespace godot;

namespace ideam::godot_ext {

void GrantRequestWindow::_bind_methods() {
    ClassDB::bind_method(D_METHOD("clear_and_popup", "node_name", "buffer_names"), &GrantRequestWindow::clear_and_popup);
    ClassDB::bind_method(D_METHOD("_on_add_part_pressed"), &GrantRequestWindow::_on_add_part_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_part_pressed", "row_container"), &GrantRequestWindow::_on_remove_part_pressed);
    ClassDB::bind_method(D_METHOD("_on_confirmed"), &GrantRequestWindow::_on_confirmed);

    ADD_SIGNAL(MethodInfo("grant_payload_submitted", 
        PropertyInfo(Variant::STRING_NAME, "node_name"), 
        PropertyInfo(Variant::PACKED_INT32_ARRAY, "selected_buffer_ids")));
}

GrantRequestWindow::GrantRequestWindow() {
    set_title("Configure Memory Grant Requirements");
    set_initial_position(WINDOW_INITIAL_POSITION_CENTER_MAIN_WINDOW_SCREEN);
    set_min_size(Vector2i(450, 400));

    // Idiomatic Godot 4 configuration:
    set_ok_button_text("Request Grant");
    add_cancel_button("Cancel");        // Adds a cancel action button directly to the dialog's platform bottom bar

    // Listen to the authoritative signal emitted when the OK/Request button is pressed
    connect("confirmed", Callable(this, "_on_confirmed"));

    // Content container
    main_layout = memnew(VBoxContainer);
    main_layout->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 8);
    add_child(main_layout);

    // Scrollable region for requirements
    parts_scroll = memnew(ScrollContainer);
    parts_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    parts_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    main_layout->add_child(parts_scroll);

    parts_container = memnew(VBoxContainer);
    parts_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    parts_scroll->add_child(parts_container);

    // Action button above the bottom bar layout
    add_part_btn = memnew(Button);
    add_part_btn->set_text("Add Grant Part");
    add_part_btn->connect("pressed", Callable(this, "_on_add_part_pressed"));
    main_layout->add_child(add_part_btn);
}

void GrantRequestWindow::clear_and_popup(const StringName& p_node_name, const TypedArray<StringName>& p_buffer_names) {
    requesting_node_name = p_node_name;
    available_buffer_names = p_buffer_names;

    // Clear previous elements
    active_option_buttons.clear();
    while (parts_container->get_child_count() > 0) {
        Node* child = parts_container->get_child(0);
        parts_container->remove_child(child);
        child->queue_free();
    }

    popup_centered_ratio();
}

void GrantRequestWindow::_on_add_part_pressed() {
    HBoxContainer* row = memnew(HBoxContainer);
    row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    parts_container->add_child(row);

    OptionButton* opt = memnew(OptionButton);
    opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    
    for (int i = 0; i < available_buffer_names.size(); ++i) {
        opt->add_item(available_buffer_names[i], i);
    }
    
    row->add_child(opt);
    active_option_buttons.append(opt);

    Button* del_btn = memnew(Button);
    del_btn->set_text(" X ");
    del_btn->connect("pressed", Callable(this, "_on_remove_part_pressed").bind(row));
    row->add_child(del_btn);
}

void GrantRequestWindow::_on_remove_part_pressed(Node* p_row_container) {
    HBoxContainer* row = Object::cast_to<HBoxContainer>(p_row_container);
    if (!row) return;

    for (int i = 0; i < row->get_child_count(); ++i) {
        OptionButton* opt = Object::cast_to<OptionButton>(row->get_child(i));
        if (opt) {
            int idx = active_option_buttons.find(opt);
            if (idx != -1) {
                active_option_buttons.remove_at(idx);
            }
            break;
        }
    }

    parts_container->remove_child(row);
    row->queue_free();
}

void GrantRequestWindow::_on_confirmed() {
    // The dialog automatically hides itself when confirmed is triggered, 
    // so we just harvest the selections and emit our downstream pipeline payload.

    godot::UtilityFunctions::print("GrantRequestWindow confirmed.");
    PackedInt32Array selected_ids;
    selected_ids.resize(active_option_buttons.size());

    for (int i = 0; i < active_option_buttons.size(); ++i) {
        OptionButton* opt = Object::cast_to<OptionButton>(active_option_buttons[i]);
        if (opt) {
            selected_ids.set(i, opt->get_selected_id());
        }
    }

    emit_signal("grant_payload_submitted", requesting_node_name, selected_ids);
}

} // namespace ideam::godot_ext