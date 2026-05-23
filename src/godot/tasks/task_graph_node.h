#pragma once

#include "../memory/memory_graph_node.h"
#include "../controls/runtime_inspector.h"
#include "task_resource.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

    // --- Fast layout struct for tracking OptionButtons awaiting async names ---
struct BufferOptionBinding {
    godot::StringName property_name;
    godot::OptionButton* button = nullptr;
    godot::PackedInt32Array buffer_ids;
};

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
    // Visual indicators
    godot::Label* task_type_label = nullptr;
    TransientWorkspaceState workspace_state = WORKSPACE_HIDDEN;

    // Internal Helpers for Theme mapping
    godot::Ref<godot::Texture2D> _get_badge_icon_for_workspace(TransientWorkspaceState p_state) const;

    // Contiguous cache of UI bindings for rapid async population
    std::vector<BufferOptionBinding> buffer_option_bindings;

    // Callable target for when the OptionButton selection changes
    void _on_buffer_option_selected(int p_index, godot::StringName p_prop_name, godot::OptionButton* p_btn);

protected:
    // The dedicated container for dynamic Tier 3 parameters (matrices, logic thresholds)
    godot::VBoxContainer* custom_parameters_container = nullptr;
    
    // Dedicated, persistent inspector for logic-specific properties
    RuntimeInspector* logic_inspector = nullptr;

    static void _bind_methods();
    
    virtual void _build_ui() override;
    
    // Intercepts draw calls to add Transient Workspace telemetry alongside memory headers
    void _notification(int p_what);

    virtual void _update_theme_properties() override;
    
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

    /**
     * @brief Recursively flattens generic "T" types and struct boundaries into concrete Variant UI instructions.
     * @param r_properties The deep-copied array of dictionaries to mutate.
     * @param p_current_type_id The currently selected structural DOD type (e.g., VECTOR3, FLOAT32).
     */
    void _reify_property_schema(godot::Array& r_properties, uint32_t p_current_type_id);
    
    /**
     * @brief Core method to build the inspector from the derived node's registry payload.
     */
    void _rebuild_logic_inspector(const godot::Array& p_properties);

    // Generalized signal router for dynamic controls connected by sub-nodes
    void _on_custom_param_changed(const godot::StringName& p_param_name, const godot::Variant& p_value);

public:
    TaskGraphNode();
    virtual ~TaskGraphNode() override = default;

    /**
     * @brief O(1) typed getter for the underlying Task Resource.
     */
    godot::Ref<TaskResource> get_task_node_resource() const;

    // Direct accessors pulling from the strictly typed Resource (No local state caching)
    uint32_t get_task_type() const;
    
    godot::StringName get_task_name() const;

    // External interface to flag heap allocation statuses (e.g., from the compiler or telemetry)
    void set_workspace_state(TransientWorkspaceState p_state);

    virtual void receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) override;

};

} // namespace ideam::godot_ext