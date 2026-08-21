#include "grant_part_view.h"

using namespace godot;

namespace ideam::godot_ext {

void GrantPartView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("populate", "data", "index"), &GrantPartView::populate);
    ClassDB::bind_method(D_METHOD("_on_inspect_pressed"), &GrantPartView::_on_inspect_pressed);
    
    ADD_SIGNAL(MethodInfo("inspect_requested", PropertyInfo(Variant::INT, "part_index")));
}

GrantPartView::GrantPartView() {
    grid = memnew(GridContainer);
    grid->set_columns(4); 
    add_child(grid);

    // Row 1
    Label* lbl_buffer = memnew(Label);
    lbl_buffer->set_text("Buffer ID:");
    grid->add_child(lbl_buffer);
    
    buffer_id_label = memnew(Label);
    buffer_id_label->set_text("-");
    grid->add_child(buffer_id_label);

    Label* lbl_access = memnew(Label);
    lbl_access->set_text("Access Mode:");
    grid->add_child(lbl_access);
    
    access_mode_label = memnew(Label);
    access_mode_label->set_text("-");
    grid->add_child(access_mode_label);

    // Row 2
    Label* lbl_count = memnew(Label);
    lbl_count->set_text("Element Count:");
    grid->add_child(lbl_count);
    
    element_count_label = memnew(Label);
    element_count_label->set_text("-");
    grid->add_child(element_count_label);

    Label* lbl_stride = memnew(Label);
    lbl_stride->set_text("Element Stride:");
    grid->add_child(lbl_stride);
    
    element_stride_label = memnew(Label);
    element_stride_label->set_text("-");
    grid->add_child(element_stride_label);

    // Row 3
    Label* lbl_capacity = memnew(Label);
    // ... [existing capacity label setup] ...
    
    // Replace filler_1 with the inspect button
    inspect_button = memnew(Button);
    inspect_button->set_text("Inspect Selection");
    inspect_button->connect("pressed", Callable(this, "_on_inspect_pressed"));
    grid->add_child(inspect_button);
    
    // Keep filler_2 to balance the 4-column grid
    Label* filler_2 = memnew(Label);
    grid->add_child(filler_2);
}

void GrantPartView::populate(const Dictionary& p_data, int p_index) {
    part_index = p_index;

    if (p_data.has("buffer_id")) {
        buffer_id_label->set_text(String::num_int64(p_data["buffer_id"]));
    }
    if (p_data.has("access_mode")) {
        access_mode_label->set_text(p_data["access_mode"]);
    }
    if (p_data.has("element_count")) {
        element_count_label->set_text(String::num_int64(p_data["element_count"]));
    }
    if (p_data.has("element_stride")) {
        element_stride_label->set_text(String::num_int64(p_data["element_stride"]));
    }
    if (p_data.has("capacity")) {
        capacity_label->set_text(String::num_int64(p_data["capacity"]));
    }
}

void GrantPartView::_on_inspect_pressed() {
    emit_signal("inspect_requested", part_index);
}

} // namespace ideam::godot_ext