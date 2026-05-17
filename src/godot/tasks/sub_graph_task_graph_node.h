#pragma once

#include "task_graph_node.h"
#include "sub_graph_task_resource.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/foldable_container.hpp>

namespace ideam::godot_ext {

class SubGraphTaskGraphNode : public TaskGraphNode {
    GDCLASS(SubGraphTaskGraphNode, TaskGraphNode)

private:
    godot::Button* child_graph_btn = nullptr;
    godot::FoldableContainer* mappings_foldable = nullptr;
    godot::VBoxContainer* mappings_list_container = nullptr;

    // --- State Caching ---
    // Persist the topography payload locally so we can dynamically add rows
    // later without re-pinging the central manager.
    godot::PackedInt32Array available_buffer_ids;
    godot::TypedArray<godot::StringName> available_buffer_names;
    godot::TypedArray<godot::StringName> available_child_nodes;

    void _add_mapping_row(int p_parent_id, const godot::StringName& p_child_node_name);
    void _sync_mappings_to_resource();

protected:
    static void _bind_methods();
    virtual void _rebuild_dynamic_ui() override;

    void _on_add_mapping_pressed();
    void _on_remove_mapping_pressed(godot::Node* p_row);
    void _on_mapping_changed(int p_index);

public:
    SubGraphTaskGraphNode();
    ~SubGraphTaskGraphNode() override = default;

    // Catches the callback from the MemoryManager (via the GraphEdit)
    virtual void receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) override;
};

} // namespace ideam::godot_ext