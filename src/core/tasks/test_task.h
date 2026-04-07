#ifndef IDEAM_CORE_TEST_TASK_H
#define IDEAM_CORE_TEST_TASK_H

#include "i_native_task.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::core {

class TestTask : public INativeTask {
public:
    virtual ~TestTask() override = default;

    // Implementation of selection culling (unused for this simple test)
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        // No culling logic needed for confirmation test
    }

    // Main execution logic
    virtual void execute(const TaskContextPOD& p_context) override {
        godot::UtilityFunctions::print("[TestTask] Execution confirmed. Delta: ", p_context.delta);
        
        // Example of using the Tier 2 command buffer to queue a change
        // This confirms the context is correctly wired
        if (p_context.wave_commands) {
             godot::UtilityFunctions::print("[TestTask] Wave command buffer is valid.");
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_TEST_TASK_H