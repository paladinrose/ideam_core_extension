#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>

#include "memory_inspectors.h"
#include "grant_part_view.h"

namespace ideam::godot_ext {

class MemoryGrantView : public godot::VBoxContainer {
    GDCLASS(MemoryGrantView, godot::VBoxContainer)

private:
    godot::Label* title_label = nullptr;
    godot::Label* status_label = nullptr;
    godot::Label* manager_version_label = nullptr;
    
    godot::VBoxContainer* parts_container = nullptr;

protected:
    static void _bind_methods();

public:
    MemoryGrantView();
    ~MemoryGrantView() = default;

    void populate_from_inspector(const godot::Ref<MemoryGrantInspector>& p_inspector);
    void clear();

    void _on_part_inspect_requested(int p_index);
};

} // namespace ideam::godot_ext