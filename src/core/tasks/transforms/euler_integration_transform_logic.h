#ifndef IDEAM_CORE_EULER_INTEGRATION_TRANSFORM_LOGIC_H
#define IDEAM_CORE_EULER_INTEGRATION_TRANSFORM_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::core {

struct alignas(64) EulerIntegrationTransformLogic {
    using ValueType       = godot::Vector3;
    using DefaultStrategy = FlatStrategy;
    // We use SwapView so we can safely read T and write T+1
    using DefaultView     = SwapView<ValueType, DefaultStrategy>;

    static constexpr TransformRequirement requirements = TransformRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr size_t transient_workspace_bytes = 0;

    // --- Configuration ---
    uint32_t position_buffer_id = INVALID_ID; // The Primary View
    uint32_t velocity_buffer_id = INVALID_ID; // The Secondary View
    float time_scale = 1.0f;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const {
        return position_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD& context, T_View& pos_view) const {
        // 1. Grab the secondary buffer (Velocity)
        const GrantPartPOD* vel_part = context.get_grant_part(velocity_buffer_id);
        if (!vel_part) return;

        // Note: In a real implementation we'd verify the selections align.
        const ValueType* velocities = reinterpret_cast<const ValueType*>(vel_part->raw_base_ptr);
        const float dt = static_cast<float>(context.delta) * time_scale;

        // 2. The Hot Loop
        const int64_t count = pos_view.count;
        for (int64_t i = 0; i < count; ++i) {
            // SwapView.get_current(i) reads the old state
            // SwapView[i] = ... writes to the new state buffer
            pos_view[i] = pos_view.get_current(i) + (velocities[i] * dt);
        }
    }
};

} // namespace ideam::core
#endif