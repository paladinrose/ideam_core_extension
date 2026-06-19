#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <godot_cpp/variant/vector3.hpp>
#include <bit>

namespace ideam::core {

/**
 * NoiseInjectionTransformLogic
 * Applies deterministic PCG hash noise to a vector array.
 * Perfect for adding turbulence to Velocity buffers.
 */
template <typename T = godot::Vector3>
struct alignas(64) NoiseInjectionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_VECTOR3;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;

    // --- Configuration ---
    uint32_t target_buffer_id = INVALID_ID;
    float magnitude = 1.0f;
    uint32_t seed = 0x4D595DF4;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mag_prop;
        mag_prop["name"] = "magnitude";
        mag_prop["type"] = godot::Variant::FLOAT;
        mag_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(mag_prop);

        godot::Dictionary seed_prop;
        seed_prop["name"] = "seed";
        seed_prop["type"] = godot::Variant::INT;
        seed_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(seed_prop);

        return props;
    }
    
    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("magnitude")) {
            magnitude = static_cast<float>(p_props["magnitude"]);
        }
        if (p_props.has("seed")) {
            seed = static_cast<uint32_t>(p_props["seed"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD& context, const T_View& main_view) const {
        GrantPartPOD* pos_part = context.get_grant_part(target_buffer_id);
        if (!pos_part) return; // Safety check
        
        const int64_t count = pos_part->selection.element_count;
        const float scale = magnitude * static_cast<float>(context.delta);
        
        // Fast temporal shifting based on engine tick
        const uint32_t temporal_seed = seed ^ (pos_part->buffer_version_at_issue * 0x1B873593);

        for (int64_t i = 0; i < count; ++i) {
            // Unroll PCG hashes for X, Y, Z
            uint32_t state_x = temporal_seed ^ static_cast<uint32_t>(i * 0x9E3779B9);
            uint32_t state_y = state_x * 0x85EBCA6B;
            uint32_t state_z = state_y * 0xC2B2AE35;

            T noise;
            noise.x = _hash_to_float_range(state_x) * scale;
            noise.y = _hash_to_float_range(state_y) * scale;
            noise.z = _hash_to_float_range(state_z) * scale;

            main_view[i] += noise;
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline float _hash_to_float_range(uint32_t& state) const {
        state ^= state >> 16;
        state *= 0x85EBCA6B;
        state ^= state >> 13;
        state *= 0xC2B2AE35;
        state ^= state >> 16;
        
        // Float conversion trick for exactly [-1.0f, 1.0f]
        uint32_t mantissa = (state & 0x007FFFFF) | 0x40000000;
        return std::bit_cast<float>(mantissa) - 3.0f; 
    }
};

} // namespace ideam::core
 // IDEAM_CORE_NOISE_INJECTION_TRANSFORM_LOGIC_H