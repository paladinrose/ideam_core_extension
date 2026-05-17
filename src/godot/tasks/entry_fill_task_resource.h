#pragma once

#include "task_resource.h"

namespace ideam::godot_ext {

/**
 * @class EntryFillTaskResource
 * @brief Strictly typed resource payload for Entry Fill tasks.
 * Designed to replace dictionary-based dynamic property lookups with a 
 * predictable memory footprint, ensuring rapid L1 cache saturation during 
 * topological validation and graph compilation.
 */
class EntryFillTaskResource : public TaskResource {
    GDCLASS(EntryFillTaskResource, TaskResource)

private:
    // 4-byte scalar. As requirements grow, group future scalars here (e.g., 
    // uint16_t flags or uint8_t masks) to maintain tight data packing and 
    // avoid compiler-injected alignment padding.
    uint32_t target_buffer_id = 0;

protected:
    static void _bind_methods();

public:
    EntryFillTaskResource() = default;
    ~EntryFillTaskResource() override = default;

    // --- Entry Configuration ---
    void set_target_buffer_id(int p_id);
    int get_target_buffer_id() const;

    godot::Dictionary get_task_properties() const override;
};

} // namespace ideam::godot_ext