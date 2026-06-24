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
template<typename T>
struct alignas(64) EulerIntegrationTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    // We use SwapView so we can safely read T and write T+1
    using DefaultView     = SwapView<ValueType, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_NUMERIC | DataType::GODOT_VECTOR_TYPES;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
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
            time_scale = static_cast<float>(p_props["time_scale"]);
        }
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(p_props["position_buffer_id"]);
        }
        if (p_props.has("velocity_buffer_id")) {
            velocity_buffer_id = static_cast<uint32_t>(p_props["velocity_buffer_id"]);
        }
    }

    template <typename T_PosView, typename T_VelView>
    inline void execute(const TaskContextPOD& context, const T_PosView& pos_view, const T_VelView& vel_view) const {
        const float dt = static_cast<float>(context.delta) * time_scale;

        // We assume pos_view dictates our iteration bounds for this transformation.
        // In a strict execution graph, context would provide the selection mask length.
        const GrantPartPOD* pos_part = context.get_grant_part(position_buffer_id);
        if (!pos_part) return;

        const int64_t count = pos_part->selection.element_count;

        // The Hot Loop: Branchless, cache-linear (if Strategy permits), and heavily optimizable by Clang/MSVC.
        for (int64_t i = 0; i < count; ++i) {
            
            // 1. Unpack Current States
            ValueType current_pos = _read_view<T_PosView>(pos_view, i);
            ValueType current_vel = _read_view<T_VelView>(vel_view, i);
            
            // 2. Execute Math
            ValueType new_pos = current_pos + (current_vel * dt);
            
            // 3. Route to Next State
            _write_view<T_PosView>(pos_view, i, new_pos);
        }
    }
    
    private:

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline ValueType _read_view(const T_View& p_view, int64_t idx) const {
        // --- DOD PROXY UNWRAPPING ---
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<ValueType>(p_view[idx].read());
        } 
        // --- ATOMIC REFERENCE UNWRAPPING ---
        else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<ValueType>(p_view[idx].load());
        }
        // --- POINTER / STANDARD RESOLUTION ---
        else {
            if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
                return *reinterpret_cast<const ValueType*>(p_view[idx]);
            } else {
                return static_cast<ValueType>(p_view[idx]);
            }
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void _write_view(const T_View& p_view, int64_t idx, const ValueType& value) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<ValueType*>(p_view[idx]) = value;
        } 
        // --- DOD PROXY WRITE ---
        // Notice we assign directly to the return of write(), or rely on the proxy's operator=
        else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value; 
        } 
        // --- ATOMIC REFERENCE STORE ---
        else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } 
        // --- STANDARD REFERENCE FALLBACK ---
        else {
            p_view[idx] = value; 
        }
    }
};

} // namespace ideam::core
