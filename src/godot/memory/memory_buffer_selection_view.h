#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/color_rect.hpp>

#include "memory_inspectors.h"

namespace ideam::godot_ext {

class MemoryBufferSelectionView : public godot::VBoxContainer {
    GDCLASS(MemoryBufferSelectionView, godot::VBoxContainer)

private:
    godot::Ref<MemorySelectionInspector> active_inspector;

    // UI Elements
    godot::OptionButton* mode_selector = nullptr;
    
    godot::HBoxContainer* range_container = nullptr;
    godot::SpinBox* start_index_spin = nullptr;
    godot::SpinBox* element_count_spin = nullptr;

    // Visual Feedback
    godot::ColorRect* interactive_visualizer = nullptr; // For drawing bitmasks/ranges
    godot::Label* conflict_warning_label = nullptr;

    bool is_updating_ui = false;

protected:
    static void _bind_methods();

public:
    MemoryBufferSelectionView();
    ~MemoryBufferSelectionView() = default;

    // Populates the read-only data and sets up the bounds
    void populate(const godot::Ref<MemorySelectionInspector>& p_inspector);
    
    // Injected by the orchestrator when a race condition is detected
    void set_conflict_state(bool p_has_conflict, const godot::String& p_message);

    // Internal UI Callbacks
    void _on_mode_selected(int p_index);
    void _on_range_changed(float p_value);
};

} // namespace ideam::godot_ext