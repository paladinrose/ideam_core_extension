#pragma once

#include "../memory/memory_graph_edit.h"
#include "../../core/tasks/registration/native_task_registry.h"
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
        godot::StringName logic_name;
    };
    std::vector<SpawnDescriptor> spawn_options_cache;

    static void _bind_methods();

    // Overrides to inject Tier 3 matrix validation and filtering
    virtual godot::TypedArray<godot::String> _get_filtered_node_types(uint32_t p_filter_mask) const override;
    
    // Intercept popup selection to build the correct dictionary structure and route to specific sub-nodes
    void _on_task_popup_select(int p_id);

    // Helper: Maps the underlying DOD MemoryStrategy to the structural BufferLayoutType
    bool _strategy_supports_layout(core::MemoryStrategy p_strategy, core::BufferLayoutType p_layout) const;

public:
    TaskGraphEdit();
    virtual ~TaskGraphEdit() override;

    void _ready() override;
};

} // namespace ideam::godot_ext