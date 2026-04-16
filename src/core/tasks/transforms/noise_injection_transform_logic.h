#ifndef IDEAM_CORE_NOISE_INJECTION_TRANSFORM_LOGIC_H
#define IDEAM_CORE_NOISE_INJECTION_TRANSFORM_LOGIC_H

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

    static constexpr TransformRequirement requirements = TransformRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY_VECTOR3;
    static constexpr size_t transient_workspace_bytes = 0;

    // --- Configuration ---
    uint32_t target_buffer_id = INVALID_ID;
    float magnitude = 1.0f;
    uint32_t seed = 0x4D595DF4;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const {
        return target_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD& context, T_View& main_view) const {
        const int64_t count = main_view.count;
        const float scale = magnitude * static_cast<float>(context.delta);
        
        // Fast temporal shifting based on engine tick
        const uint32_t temporal_seed = seed ^ (main_view.get_version() * 0x1B873593);

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
#endif // IDEAM_CORE_NOISE_INJECTION_TRANSFORM_LOGIC_H