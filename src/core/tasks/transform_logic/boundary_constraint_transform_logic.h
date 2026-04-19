#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::core {

enum class BoundaryMode : uint8_t {
    CLAMP,
    WRAP,
    BOUNCE
};

/**
 * BoundaryConstraintTransformLogic
 * Enforces spatial limits. Bouncing will automatically invert the corresponding velocity axis.
 */
template <typename T = godot::Vector3>
struct alignas(64) BoundaryConstraintTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr TransformRequirement requirements = TransformRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY_VECTOR3;
    static constexpr size_t transient_workspace_bytes = 0;

    // --- Configuration ---
    uint32_t position_buffer_id = INVALID_ID;
    uint32_t velocity_buffer_id = INVALID_ID; // Required only for BOUNCE
    
    T bounds_min;
    T bounds_max;
    BoundaryMode mode = BoundaryMode::CLAMP;
    float bounce_dampening = 0.8f;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const { return position_buffer_id; }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD& context, T_View& main_view) const {
        const MemoryBufferSelectionPOD* sel = context.get_selection(position_buffer_id);
        if (!sel) return;

        T* velocities = nullptr;
        if (mode == BoundaryMode::BOUNCE) {
            const GrantPartPOD* vel_part = context.get_grant_part(velocity_buffer_id);
            if (vel_part) velocities = reinterpret_cast<T*>(vel_part->raw_base_ptr);
        }

        const int64_t count = main_view.count;
        for (int64_t i = 0; i < count; ++i) {
            // Simple cull-check via dense array iteration
            T pos = main_view[i];
            bool modified = false;

            // X-Axis
            if (pos.x < bounds_min.x)      { pos.x = _resolve(bounds_min.x, bounds_max.x, pos.x, i, velocities, 0); modified = true; }
            else if (pos.x > bounds_max.x) { pos.x = _resolve(bounds_max.x, bounds_min.x, pos.x, i, velocities, 0); modified = true; }

            // Y-Axis
            if (pos.y < bounds_min.y)      { pos.y = _resolve(bounds_min.y, bounds_max.y, pos.y, i, velocities, 1); modified = true; }
            else if (pos.y > bounds_max.y) { pos.y = _resolve(bounds_max.y, bounds_min.y, pos.y, i, velocities, 1); modified = true; }

            // Z-Axis
            if (pos.z < bounds_min.z)      { pos.z = _resolve(bounds_min.z, bounds_max.z, pos.z, i, velocities, 2); modified = true; }
            else if (pos.z > bounds_max.z) { pos.z = _resolve(bounds_max.z, bounds_min.z, pos.z, i, velocities, 2); modified = true; }

            if (modified) main_view[i] = pos;
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline float _resolve(float limit, float wrap_target, float current, int64_t idx, T* velocities, int axis) const {
        if (mode == BoundaryMode::CLAMP) return limit;
        if (mode == BoundaryMode::WRAP) return wrap_target;
        if (mode == BoundaryMode::BOUNCE && velocities) {
            if (axis == 0) velocities[idx].x *= -bounce_dampening;
            if (axis == 1) velocities[idx].y *= -bounce_dampening;
            if (axis == 2) velocities[idx].z *= -bounce_dampening;
            return limit;
        }
        return limit;
    }
};

} // namespace ideam::core
 // IDEAM_CORE_BOUNDARY_CONSTRAINT_TRANSFORM_LOGIC_H