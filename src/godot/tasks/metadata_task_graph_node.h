#pragma once

#include "task_graph_node.h"
#include "metadata_task_graph_node_resource.h"
#include <godot_cpp/classes/option_button.hpp>

namespace ideam::godot_ext {

/**
 * @class MetadataTaskGraphNode
 * @brief Operates on a 3-dimensional memory space (View, Strategy, Type) 
 * dedicated to chunking, partitioning, and Level-of-Detail (LOD).
 */
class MetadataTaskGraphNode : public TaskGraphNode {
    GDCLASS(MetadataTaskGraphNode, TaskGraphNode)

private:
    // UI Matrix Controls
    godot::OptionButton* view_dropdown = nullptr;
    godot::OptionButton* strategy_dropdown = nullptr;
    godot::OptionButton* type_dropdown = nullptr;

    // Helpers to populate UI strings safely
    void _populate_view_dropdown();
    void _populate_strategy_dropdown();
    void _populate_type_dropdown();

protected:
    static void _bind_methods();

    virtual void _rebuild_dynamic_ui() override;
    virtual void _update_matrix_guardrails() override;
    virtual uint64_t _calculate_flat_index() const override;

    // UI Signal Handlers
    void _on_view_selected(int p_index);
    void _on_strategy_selected(int p_index);
    void _on_type_selected(int p_index);

public:
    MetadataTaskGraphNode();
    virtual ~MetadataTaskGraphNode() override = default;
};

} // namespace ideam::godot_ext