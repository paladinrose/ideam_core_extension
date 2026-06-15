#pragma once

#include "../../memory/memory_buffer_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../external/FastNoiseLite.h"

#include <godot_cpp/classes/fast_noise_lite.hpp>

#include <type_traits>

namespace ideam::core {

/* TODO:
-Finish figuring out how Godot handles noise vs domain warping. Figure out how to apply the remainder
    of the FastNoiseLite properties (especially domain warping) in a way that is both user-friendly and efficient.
- Figure out if we need to cache is_domain_warp_enabled() in the TransformLogic state to avoid branching in the inner loop.

*/
// Explicit mathematical mapping mapped to physical strides.
// The task graph must guarantee the provided memory blocks conform to this alignment.
struct FastNoiseLiteTargetPOD {
    float x;
    float y;
    float z;
    float noise_value;
};

// Alias for the raw, high-performance C++ library (global namespace)
using RealFastNoiseLite = ::FastNoiseLite;
// Alias for the Godot resource class (godot namespace)
using GodotFastNoiseLite = godot::FastNoiseLite;

struct alignas(64) FastNoiseLiteTransformLogic {
    using ValueType = FastNoiseLiteTargetPOD;
    using DefaultStrategy = FlatStrategy;
    using DefaultView = SingleElementView<ValueType, DefaultStrategy>;

    //DOD Contract Requirements
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR; // Upgraded from FLAT | SOA
    static constexpr DataType required_types              = DataType::CUSTOM;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0; // Pure mathematical mapping, no heap workspace required

    //Internal State (Trivially Copyable)
    RealFastNoiseLite fast_noise_lite;

    RealFastNoiseLite::RotationType3D rotation_type = RealFastNoiseLite::RotationType3D_None;

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

                GodotFastNoiseLite::NoiseType godot_type = setup->get_noise_type();
                RealFastNoiseLite::NoiseType type = static_cast<RealFastNoiseLite::NoiseType>(godot_type);

                fast_noise_lite.SetNoiseType(type);

                GodotFastNoiseLite::FractalType godot_fractal_type = setup->get_fractal_type();
                RealFastNoiseLite::FractalType fractal_type = static_cast<RealFastNoiseLite::FractalType>(godot_fractal_type);
                fast_noise_lite.SetFractalType(fractal_type);
                
                fast_noise_lite.SetFractalOctaves(setup->get_fractal_octaves());
                fast_noise_lite.SetFractalLacunarity(setup->get_fractal_lacunarity());
                fast_noise_lite.SetFractalGain(setup->get_fractal_gain());
                fast_noise_lite.SetFractalWeightedStrength(setup->get_fractal_weighted_strength());
                fast_noise_lite.SetFractalPingPongStrength(setup->get_fractal_ping_pong_strength());
                
                GodotFastNoiseLite::CellularDistanceFunction godot_cellular_distance_function = setup->get_cellular_distance_function();
                RealFastNoiseLite::CellularDistanceFunction cellular_distance_function = static_cast<RealFastNoiseLite::CellularDistanceFunction>(godot_cellular_distance_function);
                fast_noise_lite.SetCellularDistanceFunction(cellular_distance_function);

                GodotFastNoiseLite::CellularReturnType godot_cellular_return_type = setup->get_cellular_return_type();
                RealFastNoiseLite::CellularReturnType cellular_return_type = static_cast<RealFastNoiseLite::CellularReturnType>(godot_cellular_return_type);
                fast_noise_lite.SetCellularReturnType(cellular_return_type);
                
                fast_noise_lite.SetCellularJitter(setup->get_cellular_jitter());

                if(setup->is_domain_warp_enabled()) {
                    
                    GodotFastNoiseLite::DomainWarpType godot_domain_warp_type = setup->get_domain_warp_type();
                    RealFastNoiseLite::DomainWarpType domain_warp_type = static_cast<RealFastNoiseLite::DomainWarpType>(godot_domain_warp_type);
                    fast_noise_lite.SetDomainWarpType(domain_warp_type);
                
                    fast_noise_lite.SetDomainWarpAmp(setup->get_domain_warp_amplitude());
                    
                    float domain_warp_frequency = setup->get_domain_warp_frequency();

                    GodotFastNoiseLite::DomainWarpFractalType godot_domain_warp_fractal_type = setup->get_domain_warp_fractal_type();
                    float domain_warp_fractal_octaves = setup->get_domain_warp_fractal_octaves();

                }
            }
        }
        
        if (p_props.has("rotation_type")) {
            int rot_type_int = p_props["rotation_type"];
            rotation_type = static_cast<RealFastNoiseLite::RotationType3D>(rot_type_int);
            fast_noise_lite.SetRotationType3D(rotation_type);
        }
    }

    // Build Phase Initialization
    //FastNoiseLiteTransformLogic(int seed, float frequency, RealFastNoiseLite::NoiseType type) {
        //fast_noise_lite.SetSeed(seed);
        //fast_noise_lite.SetFrequency(frequency);
        //fast_noise_lite.SetNoiseType(type);
    //}

    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return INVALID_ID; // No input buffer, operates on generated coordinates
    }
    
    // 4. Execution Payload
    template <typename T_View>
    void execute(const TaskContextPOD& context, const T_View& view) const {
        for (auto it = view.begin(); it != view.end(); ++it) {
            FastNoiseLiteTargetPOD& target = *it;
            // Evaluates pure function mapping. No branching, no cache disruption.
            target.noise_value = fast_noise_lite.GetNoise(target.x, target.y, target.z);
        }
    }
};

// 5. Execution Pipeline Safety Guarantees
static_assert(std::is_trivially_copyable_v<FastNoiseLiteTransformLogic>, 
    "FATAL: FastNoiseLiteTransformLogic violates execution payload constraints. Struct must remain trivially copyable to map across wave batches.");
static_assert(std::is_trivially_copyable_v<RealFastNoiseLite>, 
    "FATAL: FastNoiseLite upstream modification detected. Object is no longer POD compliant and cannot be embedded in task state.");

} // namespace ideam::core

 // IDEAM_CORE_FAST_NOISE_LITE_TRANSFORM_LOGIC_H