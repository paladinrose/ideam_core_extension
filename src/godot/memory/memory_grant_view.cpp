#include "memory_grant_view.h"

using namespace godot;

namespace ideam::godot_ext {

void MemoryGrantView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("populate_from_inspector", "inspector"), &MemoryGrantView::populate_from_inspector);
    ClassDB::bind_method(D_METHOD("clear"), &MemoryGrantView::clear);
}

MemoryGrantView::MemoryGrantView() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    
    // Header Section
    HBoxContainer* header = memnew(HBoxContainer);
    add_child(header);

    title_label = memnew(Label);
    title_label->set_text("Memory Grant Profile");
    title_label->set_theme_type_variation("HeaderMedium");
    title_label->set_h_size_flags(SIZE_EXPAND_FILL);
    header->add_child(title_label);

    status_label = memnew(Label);
    status_label->set_text("Status: -");
    header->add_child(status_label);

    manager_version_label = memnew(Label);
    manager_version_label->set_text("Version: -");
    add_child(manager_version_label);
    
    // Scrollable Parts Section
    ScrollContainer* scroll = memnew(ScrollContainer);
    scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_custom_minimum_size(Vector2(0, 150)); 
    add_child(scroll);

    parts_container = memnew(VBoxContainer);
    parts_container->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(parts_container);
}

void MemoryGrantView::clear() {
    status_label->set_text("Status: -");
    manager_version_label->set_text("Version: -");
    
    for (int i = 0; i < parts_container->get_child_count(); ++i) {
        Node* child = parts_container->get_child(i);
        child->queue_free();
    }
}

void MemoryGrantView::populate_from_inspector(const Ref<MemoryGrantInspector>& p_inspector) {
    clear();
    
    if (p_inspector.is_null()) {
        return;
    }

    String status = "Active";
    if (p_inspector->has_error()) {
        status = "Error";
    } else if (p_inspector->is_emulated()) {
        status = "Emulated";
    } else if (!p_inspector->is_active()) {
        status = "Inactive";
    }
    
    if (p_inspector->is_dirty()) {
        status += " (Dirty)";
    }

    status_label->set_text("Status: " + status);
    manager_version_label->set_text("Version: " + String::num_uint64(p_inspector->get_manager_version()));

    int part_count = p_inspector->get_part_count();
    for (int i = 0; i < part_count; ++i) {
        Dictionary part_data = p_inspector->get_part_snapshot(i);
        
        GrantPartView* part_view = memnew(GrantPartView);
        parts_container->add_child(part_view);
        part_view->populate(part_data);
    }
}

} // namespace ideam::godot_ext