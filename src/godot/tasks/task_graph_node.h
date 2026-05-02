#pragma once

#include "../memory/memory_graph_node.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

/**
 * @class TaskGraphNode
 * @brief The base execution UI node. Provides the dynamic UI lifecycle, 
 * transient memory exposure, and parameter routing required by Tier 3 sub-nodes.
 */
class TaskGraphNode : public MemoryGraphNode {
    GDCLASS(TaskGraphNode, MemoryGraphNode)

public:
    enum TransientWorkspaceState {
        WORKSPACE_HIDDEN,
        WORKSPACE_ACTIVE,
        WORKSPACE_ERROR
    };

private:
    // Core task classification (Populated during _build_ui)
    uint32_t task_type = 0;
    uint32_t logic_id = 0;
    godot::StringName logic_name;

    // Visual indicators
    godot::Label* task_type_label = nullptr;
    TransientWorkspaceState workspace_state = WORKSPACE_HIDDEN;

    // Internal Helpers for Theme mapping
    godot::Ref<godot::Texture2D> _get_badge_icon_for_workspace(TransientWorkspaceState p_state) const;

protected:
    // The dedicated container for dynamic Tier 3 parameters (matrices, logic thresholds)
    godot::VBoxContainer* custom_parameters_container = nullptr;

    static void _bind_methods();
    
    virtual void _build_ui() override;
    
    // Intercepts draw calls to add Transient Workspace telemetry alongside memory headers
    void _notification(int p_what);

    // --- Tier 3 Lifecycle & Matrix Overrides (To be implemented by Sub-Nodes) ---
    
    /**
     * @brief Clears the custom_parameters_container and reconstructs the logic-specific UI.
     * Derived classes must call the base implementation FIRST to execute the teardown loop.
     */
    virtual void _rebuild_dynamic_ui();

    /**
     * @brief Validates current matrix dropdowns against the backend PackedInt64Array.
     */
    virtual void _update_matrix_guardrails();

    /**
     * @brief Calculates the flat 1D index for matrix validation based on DOD dimensionality.
     */
    virtual uint64_t _calculate_flat_index() const;

    // Generalized signal router for dynamic controls connected by sub-nodes
    void _on_custom_param_changed(const godot::StringName& p_param_name, const godot::Variant& p_value);

public:
    TaskGraphNode();
    virtual ~TaskGraphNode() override = default;

    uint32_t get_task_type() const { return task_type; }
    uint32_t get_logic_id() const { return logic_id; }
    godot::StringName get_logic_name() const { return logic_name; }

    // External interface to flag heap allocation statuses (e.g., from the compiler or telemetry)
    void set_workspace_state(TransientWorkspaceState p_state);
};

} // namespace ideam::godot_ext