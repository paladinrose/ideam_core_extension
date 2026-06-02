#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
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

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_VECTOR3; // Tightened from ANY_NUMERIC for safety
    static constexpr size_t transient_workspace_bytes     = 0;

    // --- Configuration ---
    uint32_t position_buffer_id = INVALID_ID; // The Primary View
    uint32_t velocity_buffer_id = INVALID_ID; // The Secondary View
    float time_scale = 1.0f;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary time_prop;
        time_prop["name"] = "time_scale";
        time_prop["type"] = godot::Variant::FLOAT;
        time_prop["hint"] = godot::PROPERTY_HINT_NONE;
        time_prop["hint_string"] = "suffix:x"; // Visual indicator for multiplier
        props.push_back(time_prop);

        godot::Dictionary pos_prop;
        pos_prop["name"] = "position_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary vel_prop;
        vel_prop["name"] = "velocity_buffer_id";
        vel_prop["type"] = godot::Variant::INT;
        vel_prop["hint_string"] = "buffer_option";
        props.push_back(vel_prop);

        return props;
    }
    
    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("time_scale")) {
            time_scale = p_props["time_scale"];
        }
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = p_props["position_buffer_id"];
        }
        if (p_props.has("velocity_buffer_id")) {
            velocity_buffer_id = p_props["velocity_buffer_id"];
        }
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
