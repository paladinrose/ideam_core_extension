#pragma once

#include "task_graph_node.h"
#include "query_task_resource.h"
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>

namespace ideam::godot_ext {

/**
 * @class QueryTaskGraphNode
 * @brief The most complex node, operating on a 4-dimensional memory space.
 * Features an explicit QueryOp control that dictates Operational Topology 
 * (Synchronous CULL vs Deferred ADD) and frequent MutativeBridge evaluations.
 */
class QueryTaskGraphNode : public TaskGraphNode {
    GDCLASS(QueryTaskGraphNode, TaskGraphNode)

private:
    // UI Matrix Controls
    godot::OptionButton* op_dropdown = nullptr;
    godot::OptionButton* view_dropdown = nullptr;
    godot::OptionButton* strategy_dropdown = nullptr;
    godot::OptionButton* type_dropdown = nullptr;

    // Helpers to populate UI strings safely
    void _populate_op_dropdown();
    void _populate_view_dropdown();
    void _populate_strategy_dropdown();
    void _populate_type_dropdown();

protected:
    static void _bind_methods();
    virtual void _rebuild_dynamic_ui() override;
    virtual void _update_matrix_guardrails() override;
    virtual uint64_t _calculate_flat_index() const override;

    void _rebuild_ports();

    // UI Signal Handlers
    void _on_op_selected(int p_index);
    void _on_view_selected(int p_index);
    void _on_strategy_selected(int p_index);
    void _on_type_selected(int p_index);

public:
    QueryTaskGraphNode();
    virtual ~QueryTaskGraphNode() override = default;
};

} // namespace ideam::godot_ext