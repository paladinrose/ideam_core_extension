#include "memory_buffer_selection_view.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryBufferSelectionView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("populate", "inspector"), &MemoryBufferSelectionView::populate);
    ClassDB::bind_method(D_METHOD("set_conflict_state", "has_conflict", "message"), &MemoryBufferSelectionView::set_conflict_state);
    
    ClassDB::bind_method(D_METHOD("_on_mode_selected", "index"), &MemoryBufferSelectionView::_on_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_range_changed", "value"), &MemoryBufferSelectionView::_on_range_changed);

    ADD_SIGNAL(MethodInfo("selection_modified")); 
}

MemoryBufferSelectionView::MemoryBufferSelectionView() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 8);

    // --- Header / Mode Selector ---
    HBoxContainer* header_box = memnew(HBoxContainer);
    add_child(header_box);

    Label* mode_label = memnew(Label);
    mode_label->set_text("Selection Mode:");
    header_box->add_child(mode_label);

    mode_selector = memnew(OptionButton);
    mode_selector->add_item("Sparse (ID List)", 0);
    mode_selector->add_item("Dense (Bitmask)", 1);
    mode_selector->add_item("Range (Contiguous)", 2);
    mode_selector->set_h_size_flags(SIZE_EXPAND_FILL);
    header_box->add_child(mode_selector);

    mode_selector->connect("item_selected", Callable(this, "_on_mode_selected"));

    // --- Range Controls (Visible primarily when MODE = RANGE) ---
    range_container = memnew(HBoxContainer);
    add_child(range_container);

    Label* start_label = memnew(Label);
    start_label->set_text("Start Index:");
    range_container->add_child(start_label);

    start_index_spin = memnew(SpinBox);
    start_index_spin->set_max(1000000); // Will be clamped by capacity in populate()
    range_container->add_child(start_index_spin);

    Label* count_label = memnew(Label);
    count_label->set_text("Count:");
    range_container->add_child(count_label);

    element_count_spin = memnew(SpinBox);
    element_count_spin->set_max(1000000); 
    range_container->add_child(element_count_spin);

    start_index_spin->connect("value_changed", Callable(this, "_on_range_changed"));
    element_count_spin->connect("value_changed", Callable(this, "_on_range_changed"));

    // --- Interactive Visualizer Placeholder ---
    // A canvas area where we can eventually custom-draw overlapping bounds or bitmasks
    interactive_visualizer = memnew(ColorRect);
    interactive_visualizer->set_custom_minimum_size(Vector2(0, 30));
    interactive_visualizer->set_h_size_flags(SIZE_EXPAND_FILL);
    interactive_visualizer->set_color(Color(0.2, 0.2, 0.2, 1.0)); // Dark gray backdrop
    add_child(interactive_visualizer);

    // --- Conflict Warning Label ---
    conflict_warning_label = memnew(Label);
    conflict_warning_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    conflict_warning_label->set_visible(false); // Hidden by default
    conflict_warning_label->add_theme_color_override("font_color", Color(1.0, 0.4, 0.4, 1.0)); // Red warning text
    add_child(conflict_warning_label);
}

void MemoryBufferSelectionView::populate(const Ref<MemorySelectionInspector>& p_inspector) {
    if (p_inspector.is_null()) return;

    active_inspector = p_inspector;
    is_updating_ui = true;

    // Convert the string-based enum mapping back to UI indices[cite: 6]
    String mode_str = active_inspector->get_selection_mode_string();
    if (mode_str.contains("Sparse")) {
        mode_selector->select(0);
        range_container->set_visible(false);
    } else if (mode_str.contains("Dense")) {
        mode_selector->select(1);
        range_container->set_visible(false);
    } else {
        mode_selector->select(2);
        range_container->set_visible(true);
    }

    // Set capacity limits
    // Note: Assuming MemorySelectionInspector gets a get_capacity() method from our previous context updates
    // int capacity = active_inspector->get_capacity();
    // start_index_spin->set_max(capacity);
    // element_count_spin->set_max(capacity);

    element_count_spin->set_value(active_inspector->get_element_count());
    
    // Clear previous conflicts on fresh populate
    set_conflict_state(false, "");

    is_updating_ui = false;
}

void MemoryBufferSelectionView::set_conflict_state(bool p_has_conflict, const String& p_message) {
    conflict_warning_label->set_visible(p_has_conflict);
    conflict_warning_label->set_text(p_message);

    if (p_has_conflict) {
        interactive_visualizer->set_color(Color(0.5, 0.1, 0.1, 1.0)); // Tint red
    } else {
        interactive_visualizer->set_color(Color(0.2, 0.2, 0.2, 1.0)); // Back to normal
    }
}

void MemoryBufferSelectionView::_on_mode_selected(int p_index) {
    if (is_updating_ui) return;
    
    range_container->set_visible(p_index == 2); // Only show Range spins if RANGE mode
    emit_signal("selection_modified");
}

void MemoryBufferSelectionView::_on_range_changed(float p_value) {
    if (is_updating_ui) return;
    emit_signal("selection_modified");
}

} // namespace ideam::godot_ext