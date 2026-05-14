#pragma once

#include "task_graph_node.h"
#include "entry_fill_task_resource.h"
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

/**
 * @class EntryFillTaskGraphNode
 * @brief UI representation for the EntryFillTask. Requests and displays 
 * all available memory buffers. Leverages the 1:1 architectural guarantee 
 * that the list index intrinsically represents the hardware buffer ID.
 */
class EntryFillTaskGraphNode : public TaskGraphNode {
    GDCLASS(EntryFillTaskGraphNode, TaskGraphNode)

private:
    godot::OptionButton* buffer_dropdown = nullptr;

protected:
    static void _bind_methods();

    virtual void _rebuild_dynamic_ui() override;

    // UI Signal Handlers
    void _on_buffer_selected(int p_index);

public:
    EntryFillTaskGraphNode();
    ~EntryFillTaskGraphNode() override = default;

    /**
     * @brief Overrides the MemoryGraphNode virtual. Receives the requested 
     * list of buffer names from the central registry and populates the UI.
     * @param p_names Godot TypedArray of StringNames for UI display.
     */
    virtual void receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) override;
};

} // namespace ideam::godot_ext