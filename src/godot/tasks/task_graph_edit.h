#pragma once

#include "../memory/memory_graph_edit.h"
#include "../../core/tasks/registration/native_task_registry.h"
#include "task_resource.h"
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>

namespace ideam::godot_ext {

/**
 * @class TaskGraphEdit
 * @brief The Tier 3 execution topography editor. 
 * Interrogates NativeTaskRegistry matrices to enforce O(1) visual guardrails,
 * ensuring users can only construct memory-safe topological paths.
 */
class TaskGraphEdit : public MemoryGraphEdit {
    GDCLASS(TaskGraphEdit, MemoryGraphEdit)

public:
    enum TaskCategory : uint32_t {
        CATEGORY_TRANSFORM = 0,
        CATEGORY_METADATA,
        CATEGORY_QUERY,
        CATEGORY_MANUAL
    };

protected:
    // --- O(1) Context Menu Cache ---
    // Maps the flat int ID emitted by PopupMenu directly to the required DOD instantiation logic.
    // std::vector guarantees contiguous memory for optimal cache prefetching during UI population.
    struct SpawnDescriptor {
        TaskCategory category;
        uint32_t logic_id;
        godot::StringName task_name;
    };
    std::vector<SpawnDescriptor> spawn_options_cache;

    // The active mask for context-aware port dragging. 
    // Default to 0 (No filter) for generic canvas right-clicks.
    uint32_t active_filter_mask = 0; 

    static void _bind_methods();
    virtual void _notification(int p_what) override;
    virtual void _update_theme_properties() override;

    // Overrides to build our hierarchical popup and instantiate DOD nodes
    virtual godot::TypedArray<godot::String> _get_new_node_types() const override;
    virtual void _spawn_node_by_type(int p_type_id) override;
    virtual IdeamGraphNode* _create_graph_node(const godot::Ref<IdeamGraphNodeResource>& p_node_res) override;
    
    // Helper: Maps the underlying DOD MemoryStrategy to the structural BufferLayoutType
    bool _strategy_supports_layout(core::MemoryStrategy p_strategy, core::BufferLayoutType p_layout) const;

public:
    TaskGraphEdit();
    virtual ~TaskGraphEdit() override;

    void _ready() override;
};

} // namespace ideam::godot_ext