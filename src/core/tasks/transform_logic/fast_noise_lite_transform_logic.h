#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../external/FastNoiseLite.h"

#include <type_traits>

namespace ideam::core {

// Explicit mathematical mapping mapped to physical strides.
// The task graph must guarantee the provided memory blocks conform to this alignment.
struct FastNoiseLiteTargetPOD {
    float x;
    float y;
    float z;
    float noise_value;
};

struct alignas(64) FastNoiseLiteTransformLogic {
    using ValueType = FastNoiseLiteTargetPOD;
    using DefaultStrategy = FlatStrategy;
    using DefaultView = SingleElementView<ValueType, DefaultStrategy>;

    //DOD Contract Requirements
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR; // Upgraded from FLAT | SOA
    static constexpr DataType required_types              = DataType::CUSTOM;
    static constexpr size_t transient_workspace_bytes     = 0; // Pure mathematical mapping, no heap workspace required

    //Internal State (Trivially Copyable)
    FastNoiseLite noise_generator;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary noise_prop;
        noise_prop["name"] = "noise_setup";
        noise_prop["type"] = godot::Variant::OBJECT;
        noise_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        noise_prop["hint_string"] = "FastNoiseLite";
        props.push_back(noise_prop);

        return props;
    }
    
    // Build Phase Initialization
    FastNoiseLiteTransformLogic(int seed, float frequency, FastNoiseLite::NoiseType type) {
        noise_generator.SetSeed(seed);
        noise_generator.SetFrequency(frequency);
        noise_generator.SetNoiseType(type);
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return INVALID_ID; // No input buffer, operates on generated coordinates
    }
    
    // 4. Execution Payload
    template <typename T_View>
    void execute_transform(const TaskContextPOD& context, T_View& view) const {
        for (auto it = view.begin(); it != view.end(); ++it) {
            FastNoiseLiteTargetPOD& target = *it;
            // Evaluates pure function mapping. No branching, no cache disruption.
            target.noise_value = noise_generator.GetNoise(target.x, target.y, target.z);
        }
    }
};

// 5. Execution Pipeline Safety Guarantees
static_assert(std::is_trivially_copyable_v<FastNoiseLiteTransformLogic>, 
    "FATAL: FastNoiseLiteTransformLogic violates execution payload constraints. Struct must remain trivially copyable to map across wave batches.");
static_assert(std::is_trivially_copyable_v<FastNoiseLite>, 
    "FATAL: FastNoiseLite upstream modification detected. Object is no longer POD compliant and cannot be embedded in task state.");

} // namespace ideam::core

 // IDEAM_CORE_FAST_NOISE_LITE_TRANSFORM_LOGIC_H