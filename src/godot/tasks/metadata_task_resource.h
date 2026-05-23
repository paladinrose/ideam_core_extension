#pragma once

#include "task_resource.h"

namespace ideam::godot_ext {

/**
 * @class MetadataTaskResource
 * @brief Strictly typed resource payload for Metadata execution nodes.
 * Replaces dynamic dictionaries with packed struct members to guarantee 
 * predictable cache line alignment during DOD graph compilation.
 */
class MetadataTaskResource : public TaskResource {
    GDCLASS(MetadataTaskResource, TaskResource)

private:
    // Tightly packed integer states for O(1) DOD matrix resolution.
    // Abstracted from Variant/Dictionary containers to prevent heap fragmentation.
    uint32_t view_id = 0;
    uint32_t strategy_id = 0;
    uint32_t type_id = 0;
    uint32_t logic_id = 0;

protected:
    static void _bind_methods();

public:
    MetadataTaskResource() = default;
    ~MetadataTaskResource() override = default;

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