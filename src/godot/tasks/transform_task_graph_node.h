#pragma once

#include "task_graph_node.h"
#include "transform_task_resource.h"
#include <godot_cpp/classes/option_button.hpp>

namespace ideam::godot_ext {

/**
 * @class TransformTaskGraphNode
 * @brief Operates on a strictly 3-dimensional memory space mapping data transformations.
 * Enforces ViewCapability interrogations to ensure logic topologies match structural constraints.
 */
class TransformTaskGraphNode : public TaskGraphNode {
    GDCLASS(TransformTaskGraphNode, TaskGraphNode)

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
    TransformTaskGraphNode();
    virtual ~TransformTaskGraphNode() override = default;

    godot::Ref<TransformTaskResource> get_transform_task_resource() const;
    
    uint32_t get_logic_id() const;
};

} // namespace ideam::godot_ext