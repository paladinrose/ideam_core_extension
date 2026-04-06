#ifndef IDEAM_GODOT_TASK_GRAPH_EDIT_H
#define IDEAM_GODOT_TASK_GRAPH_EDIT_H

#include "../memory/memory_graph_edit.h"

namespace godot {

class TaskGraphEdit : public MemoryGraphEdit {
    GDCLASS(TaskGraphEdit, MemoryGraphEdit)

protected:
    static void _bind_methods();

    // Overrides to inject Task-specific instantiation logic
    virtual TypedArray<String> _get_filtered_node_types(uint32_t p_filter_mask) const override;
    
    // Intercept popup selection to build the correct dictionary structure
    void _on_task_popup_select(int p_id);

public:
    TaskGraphEdit();
    virtual ~TaskGraphEdit() override;

    void _ready() override;
};

} // namespace godot

#endif // IDEAM_GODOT_TASK_GRAPH_EDIT_H