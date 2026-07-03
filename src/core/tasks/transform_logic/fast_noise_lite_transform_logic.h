#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../../memory/vector_traits.h" // Brought in for compile-time structural inspection
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../external/FastNoiseLite.h"

// Keep GLM just to satisfy the DefaultView baseline, but the execution pipeline is now math-library agnostic
#include "../../../external/glm/vec3.hpp"

#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <type_traits>

namespace ideam::core {

using RealFastNoiseLite = ::FastNoiseLite;
using GodotFastNoiseLite = godot::FastNoiseLite;
template <typename T = godot::Vector3>
struct alignas(64) FastNoiseLiteTransformLogic {
    using ValueType = godot::Vector3; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView = SingleElementView<ValueType, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR; 
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3;
    
    static constexpr size_t dimensions = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    static constexpr size_t transient_workspace_bytes = 0; 

    static constexpr std::string_view display_name = "FastNoiseLite";
    
    RealFastNoiseLite fast_noise_lite;
    RealFastNoiseLite::RotationType3D rotation_type = RealFastNoiseLite::RotationType3D_None;

    uint32_t position_buffer_id = INVALID_ID;
    uint32_t noise_buffer_id    = INVALID_ID;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary noise_prop;
        noise_prop["name"] = "noise_setup";
        noise_prop["type"] = godot::Variant::OBJECT;
        noise_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        noise_prop["hint_string"] = "FastNoiseLite";
        props.push_back(noise_prop);

        godot::Dictionary rotation_prop;
        rotation_prop["name"] = "rotation_type";
        rotation_prop["type"] = godot::Variant::INT;
        rotation_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        rotation_prop["hint_string"] = "None,Improved,Best";
        props.push_back(rotation_prop);

        return props;
    }
    
    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("noise_setup")) {
            godot::Ref<GodotFastNoiseLite> setup = p_props["noise_setup"];
            if (setup.is_valid()) {
                fast_noise_lite.SetSeed(setup->get_seed());
                fast_noise_lite.SetFrequency(setup->get_frequency());

                fast_noise_lite.SetNoiseType(static_cast<RealFastNoiseLite::NoiseType>(setup->get_noise_type()));
                fast_noise_lite.SetFractalType(static_cast<RealFastNoiseLite::FractalType>(setup->get_fractal_type()));
                
                fast_noise_lite.SetFractalOctaves(setup->get_fractal_octaves());
                fast_noise_lite.SetFractalLacunarity(setup->get_fractal_lacunarity());
                fast_noise_lite.SetFractalGain(setup->get_fractal_gain());
                fast_noise_lite.SetFractalWeightedStrength(setup->get_fractal_weighted_strength());
                fast_noise_lite.SetFractalPingPongStrength(setup->get_fractal_ping_pong_strength());
                
                fast_noise_lite.SetCellularDistanceFunction(static_cast<RealFastNoiseLite::CellularDistanceFunction>(setup->get_cellular_distance_function()));
                fast_noise_lite.SetCellularReturnType(static_cast<RealFastNoiseLite::CellularReturnType>(setup->get_cellular_return_type()));
                fast_noise_lite.SetCellularJitter(setup->get_cellular_jitter());

                if(setup->is_domain_warp_enabled()) {
                    fast_noise_lite.SetDomainWarpType(static_cast<RealFastNoiseLite::DomainWarpType>(setup->get_domain_warp_type()));
                    fast_noise_lite.SetDomainWarpAmp(setup->get_domain_warp_amplitude());
                    
                    // Note: Frequency and fractal settings for domain warp are separated in Godot's API but currently 
                    // unsupported by the raw FastNoiseLite header without passing a warp object. Handled below minimally.
                }
            }
        }
        
        if (p_props.has("rotation_type")) {
            rotation_type = static_cast<RealFastNoiseLite::RotationType3D>(static_cast<int>(p_props["rotation_type"]));
            fast_noise_lite.SetRotationType3D(rotation_type);
        }
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return noise_buffer_id;
    }
    
    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& context, const T_View& pos_view) const {
        const GrantPartPOD* pos_part = context.get_grant_part(position_buffer_id);
        const GrantPartPOD* noise_part = context.get_grant_part(noise_buffer_id);
        
        if (!pos_part || !noise_part) return;
        const int64_t count = pos_part->selection.element_count;

        using NoiseView = SingleElementView<float, T_Strategy>;
        NoiseView noise_view;
        
        noise_view.bind(noise_part);

        
        // C++20 Concept Barrier: Refuse compilation if the incoming view is not a spatial vector
        static_assert(HasXAndY<T>, "FATAL: FastNoiseLite stream strictly requires a geometric vector input (HasXAndY concept failed).");

        // The Hot Loop: Zero abstraction overhead
        for (int64_t i = 0; i < count; ++i) {
            const T& pos = _read_view(pos_view, i);
            float noise_val = 0.0f;

            // Constexpr branching relies on structural validity mapped via your traits
            if constexpr (requires { pos.z; }) {
                noise_val = fast_noise_lite.GetNoise(
                    static_cast<float>(pos.x), 
                    static_cast<float>(pos.y), 
                    static_cast<float>(pos.z)
                );
            } else {
                noise_val = fast_noise_lite.GetNoise(
                    static_cast<float>(pos.x), 
                    static_cast<float>(pos.y)
                );
            }

            noise_view[i] = noise_val;
        }
    }

private:

template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const {
        // --- DOD PROXY UNWRAPPING ---
        // Statically detects if the View returns a proxy object
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } 
        // --- ATOMIC REFERENCE UNWRAPPING ---
        else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        }
        // --- STANDARD RESOLUTION ---
        else {
            if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
                // We bypass intermediate decay and cast the generic buffer pointer directly to T*.
                // This ensures we read the full sizeof(T) block from the cache line in a single fetch.
                return *reinterpret_cast<const T*>(p_view[idx]);
            } else {
                // If it's a value or a reference proxy, invoke its conversion operator.
                return static_cast<T>(p_view[idx]);
            }
        }
    }
};

static_assert(std::is_trivially_copyable_v<RealFastNoiseLite>, 
    "FATAL: FastNoiseLite upstream modification detected.");

} // namespace ideam::core