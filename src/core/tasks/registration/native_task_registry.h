#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"     // For DataType
#include "../../memory/memory_buffer_pod.h" // For BufferLayoutType
#include "../../memory/views/view_traits.h"       // For ViewStrategies and ViewCapability

// --- Centralized Enums & Traits (Required by Sub-Registries) ---
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <memory>
#include <functional>
#include <cstdint>

namespace ideam::core {

enum class MemoryView : uint32_t {
    SingleElementView = 0,
    MultiElementView,
    SparseSetView,
    PagedView,
    RingView,
    StencilView,
    AtomicView,
    SwapView,
    StaticStencilView,
    BridgeView,
    AOSOA_Tight_AVX2,
    AOSOA_Tight_AVX512,
    AOSOA_STD430_AVX2,
    AOSOA_STD430_AVX512,
    AOSOA_STD140_AVX2,
    AOSOA_STD140_AVX512,
    Count
};

enum class MemoryStrategy {
    FlatStrategy,
    SoAStrategy,
    AoSStrategy,
    Spatial2DStrategy,
    Spatial3DStrategy,
    Spatial4DStrategy,
    TiledSoAStrategy,
    RingStrategy,
    PagedStrategy, 
    Count
};

enum class MemoryTypes {
    BOOL = 0,
    BYTE,
    INT32,
    INT64,
    FLOAT32,
    FLOAT64,
    VECTOR2,
    VECTOR3,
    VECTOR4,
    VECTOR2I,
    VECTOR3I,
    VECTOR4I,
    VECTOR2D,
    VECTOR3D,
    VECTOR4D,
    COLOR,
    CUSTOM,
    Count
};

// --- The Translation Layer ---
template <MemoryTypes T_MemType>
struct NativeMemoryTraits {
    // Base template left deliberately undefined to enforce explicit specialization
};

// --- Specializations Mapping MemoryTypes -> Concrete C++ Type & DataType Bitmask ---
template <> struct NativeMemoryTraits<MemoryTypes::BOOL> { using ConcreteType = bool; static constexpr DataType DataFlag = DataType::BOOL; };
template <> struct NativeMemoryTraits<MemoryTypes::BYTE> { using ConcreteType = uint8_t; static constexpr DataType DataFlag = DataType::BYTE; };
template <> struct NativeMemoryTraits<MemoryTypes::INT32> { using ConcreteType = int32_t; static constexpr DataType DataFlag = DataType::INT32; };
template <> struct NativeMemoryTraits<MemoryTypes::INT64> { using ConcreteType = int64_t; static constexpr DataType DataFlag = DataType::INT64; };
template <> struct NativeMemoryTraits<MemoryTypes::FLOAT32> { using ConcreteType = float; static constexpr DataType DataFlag = DataType::FLOAT32; };
template <> struct NativeMemoryTraits<MemoryTypes::FLOAT64> { using ConcreteType = double; static constexpr DataType DataFlag = DataType::FLOAT64; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2> { using ConcreteType = godot::Vector2; static constexpr DataType DataFlag = DataType::VECTOR2; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3> { using ConcreteType = godot::Vector3; static constexpr DataType DataFlag = DataType::VECTOR3; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4> { using ConcreteType = godot::Vector4; static constexpr DataType DataFlag = DataType::VECTOR4; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2I> { using ConcreteType = godot::Vector2i; static constexpr DataType DataFlag = DataType::VECTOR2I; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3I> { using ConcreteType = godot::Vector3i; static constexpr DataType DataFlag = DataType::VECTOR3I; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4I> { using ConcreteType = godot::Vector4i; static constexpr DataType DataFlag = DataType::VECTOR4I; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2D> { using ConcreteType = godot::Vector2; static constexpr DataType DataFlag = DataType::VECTOR2D; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3D> { using ConcreteType = godot::Vector3; static constexpr DataType DataFlag = DataType::VECTOR3D; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4D> { using ConcreteType = godot::Vector4; static constexpr DataType DataFlag = DataType::VECTOR4D; };
template <> struct NativeMemoryTraits<MemoryTypes::COLOR> { using ConcreteType = godot::Color; static constexpr DataType DataFlag = DataType::COLOR; };
template <> struct NativeMemoryTraits<MemoryTypes::CUSTOM> { using ConcreteType = void*; static constexpr DataType DataFlag = DataType::CUSTOM; };



using NativeTaskFactory = std::function<std::unique_ptr<INativeTask>()>;

class NativeTaskRegistry {
private:
    // Only retains tasks that require dynamic/hashed lookups (e.g., manual overrides or entry tasks)
    static godot::HashMap<godot::StringName, NativeTaskFactory>* manual_factories;
    // The structural manifest for Godot UI
    static godot::Dictionary* ui_utility_matrix;

public:
    // --- Lifecycle Management ---
    static void init();
    static void cleanup();

    // --- Dynamic Creation for Manual Tasks ---
    [[nodiscard]] static std::unique_ptr<INativeTask> create(const godot::StringName& p_name);

    // --- Variadic Registration (Added Resource and GraphNode template constraints) ---
    template <typename T_Task, typename T_Resource, typename T_Node, typename... Args>
    static void register_task(const godot::StringName& p_name, Args... args) {
        if (!manual_factories) return; 

        // 1. Register Execution Strategy
        (*manual_factories)[p_name] = [args...]() -> std::unique_ptr<INativeTask> {
            return std::make_unique<T_Task>(args...);
        };

        // 2. Register UI Presentation Data
        if (ui_utility_matrix) {
            godot::Dictionary task_def;
            
            // Storing the static class name references allows for generic dynamic spawning in the editor
            task_def["resource_class"] = T_Resource::get_class_static();
            task_def["node_class"] = T_Node::get_class_static();
            
            // Manual tasks (like SubGraphTask) generally handle layout routing internally 
            // or perform structural operations. By omitting the "valid_combinations" key, 
            // we implicitly signal to TaskGraphEdit that this node bypasses the strict 
            // hardware-layout filter and should always be available in the Utility menu.
            
            (*ui_utility_matrix)[p_name] = task_def;
        }
    }
    
    // --- Legacy Godot Accessors (Routing directly to specialized registries to avoid state duplication) ---
    static godot::Dictionary get_ui_query_matrix();
    static godot::Dictionary get_ui_transform_matrix();
    static godot::Dictionary get_ui_metadata_matrix();
    static godot::Dictionary get_ui_simulation_matrix();
    static godot::Dictionary get_ui_utility_matrix();
};

} // namespace ideam::core