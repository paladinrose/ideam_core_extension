#pragma once

#include "../../memory/memory_manager_dod.h"
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

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_VECTOR3;
    static constexpr size_t transient_workspace_bytes     = 0;

    // --- Configuration ---
    uint32_t position_buffer_id = INVALID_ID;
    uint32_t velocity_buffer_id = INVALID_ID; // Required only for BOUNCE
    
    T bounds_min;
    T bounds_max;
    BoundaryMode mode = BoundaryMode::CLAMP;
    float bounce_dampening = 0.8f;

    static godot::Array get_ui_properties() {
        godot::Array props;

        // 1. mode (Native Godot Enum Dropdown)
        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Clamp,Wrap,Bounce";
        props.push_back(mode_prop);

        // --- Dynamic Vector Type Hints ---
        // Since 'T' can be any Vector3 variant, we provide detailed type hints for the Inspector.
        // Godot natively handles Vectors, but specifying the exact Variant type 
        // and step/suffix hints makes the Inspector much more robust.
        godot::Dictionary type_hints;

        // Hint for Floating-Point Vectors
        godot::Dictionary float_vec_hint;
        float_vec_hint["type"] = godot::Variant::VECTOR3; // Explicitly map to Godot's Float Vector
        float_vec_hint["hint"] = godot::PROPERTY_HINT_NONE;
        float_vec_hint["hint_string"] = "suffix:m"; // Optional: Adds 'm' (meters) to the UI boxes
        
        // Hint for Integer Vectors
        godot::Dictionary int_vec_hint;
        int_vec_hint["type"] = godot::Variant::VECTOR3I; // Explicitly map to Godot's Int Vector
        int_vec_hint["hint"] = godot::PROPERTY_HINT_NONE;
        int_vec_hint["hint_string"] = ""; 

        type_hints[static_cast<uint32_t>(MemoryTypes::VECTOR3)] = float_vec_hint;
        type_hints[static_cast<uint32_t>(MemoryTypes::VECTOR3D)] = float_vec_hint;
        type_hints[static_cast<uint32_t>(MemoryTypes::VECTOR3I)]   = int_vec_hint;

        // 2. bounds_min (Dynamic Type 'T')
        godot::Dictionary min_prop;
        min_prop["name"] = "bounds_min";
        min_prop["type"] = "T"; 
        min_prop["type_hints"] = type_hints;
        props.push_back(min_prop);

        // 3. bounds_max (Dynamic Type 'T')
        godot::Dictionary max_prop;
        max_prop["name"] = "bounds_max";
        max_prop["type"] = "T";
        max_prop["type_hints"] = type_hints;
        props.push_back(max_prop);

        return props;
    }
    
    [[nodiscard]] inline uint32_t get_target_buffer_id() const { return position_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("mode")) {
            mode = static_cast<BoundaryMode>(static_cast<uint8_t>(p_props["mode"]));
        }
        if (p_props.has("bounds_min")) {
            bounds_min = p_props["bounds_min"];
        }
        if (p_props.has("bounds_max")) {
            bounds_max = p_props["bounds_max"];
        }
    }
    
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