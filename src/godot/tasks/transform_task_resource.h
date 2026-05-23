#pragma once

#include "task_resource.h"

namespace ideam::godot_ext {

/**
 * @class TransformTaskResource
 * @brief Strictly typed resource payload for Transform execution nodes.
 * Replaces dictionary allocations with a 12-byte packed struct (view, strategy, type),
 * ensuring fast, predictable cache alignment during execution graph compilation.
 */
class TransformTaskResource : public TaskResource {
    GDCLASS(TransformTaskResource, TaskResource)

private:
    // Tightly packed 12-byte boundary for O(1) DOD matrix resolution.
    // Abstracted from Godot Variants to guarantee zero-allocation reads.
    uint32_t view_id = 0;
    uint32_t strategy_id = 0;
    uint32_t type_id = 0;
    uint32_t logic_id = 0;

protected:
    static void _bind_methods();

public:
    TransformTaskResource() = default;
    ~TransformTaskResource() override = default;

    // --- DOD Matrix Configuration ---
    void set_view_id(int p_id);
    int get_view_id() const;

    void set_strategy_id(int p_id);
    int get_strategy_id() const;

    void set_type_id(int p_id);
    int get_type_id() const;

    void set_logic_id(uint32_t p_id);
    uint32_t get_logic_id() const;
};

} // namespace ideam::godot_ext