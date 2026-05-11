#pragma once

#include "task_graph_node_resource.h"

namespace ideam::godot_ext {

/**
 * @class QueryTaskGraphNodeResource
 * @brief Strictly typed resource payload for Query execution nodes.
 * Replaces dictionary allocations with a 16-byte packed struct (op, view, strategy, type),
 * maximizing L1 cache saturation and avoiding pointer chasing during topological validation.
 */
class QueryTaskGraphNodeResource : public TaskGraphNodeResource {
    GDCLASS(QueryTaskGraphNodeResource, TaskGraphNodeResource)

private:
    // Tightly packed 16-byte boundary. O(1) fetch during graph compilation.
    uint32_t op_id = 0;
    uint32_t view_id = 0;
    uint32_t strategy_id = 0;
    uint32_t type_id = 0;

protected:
    static void _bind_methods();

public:
    QueryTaskGraphNodeResource() = default;
    ~QueryTaskGraphNodeResource() override = default;

    // --- DOD Matrix Configuration ---
    void set_op_id(int p_id);
    int get_op_id() const;

    void set_view_id(int p_id);
    int get_view_id() const;

    void set_strategy_id(int p_id);
    int get_strategy_id() const;

    void set_type_id(int p_id);
    int get_type_id() const;
};

} // namespace ideam::godot_ext