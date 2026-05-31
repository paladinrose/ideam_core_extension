#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"           // For DataType
#include "../../memory/memory_buffer_pod.h"       // For BufferLayoutType
#include "../../memory/views/view_traits.h"       // For ViewStrategies and ViewCapability

#include "../../../godot/tasks/task_resource.h" 
#include "../../../godot/tasks/task_graph_node.h" 
#include "task_manifest.h"

#include <godot_cpp/core/object.hpp>
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

struct TaskUIFactories {
    std::function<godot::Ref<godot_ext::TaskResource>()> resource_factory;
    std::function<godot_ext::TaskGraphNode*()> node_factory;
};

using NativeTaskFactory = std::function<std::unique_ptr<INativeTask>()>;

class IdeamTaskRegistry : public godot::Object {
    GDCLASS(IdeamTaskRegistry, godot::Object)

private:
    // Global Singleton Reference
    static IdeamTaskRegistry* singleton;

    // Runtime C++ Memory Allocation (Not Serialized)
    godot::HashMap<godot::StringName, NativeTaskFactory> manual_factories;
    godot::HashMap<godot::StringName, TaskUIFactories> ui_factories;

    // Temporary storage for utility definitions pending bake
    godot::Dictionary pending_utility_matrix;

    // Active loaded manifest
    godot::Ref<TaskManifest> active_manifest;

protected:
    static void _bind_methods();

public:
    IdeamTaskRegistry() = default;
    ~IdeamTaskRegistry() = default;

    // --- Lifecycle Management ---
    static void init();
    static void cleanup();

    // --- Singleton Accessors ---
    static IdeamTaskRegistry* get_singleton() { return singleton; }

    // --- Task Baking ---
    void bake_manifest();

    // --- Active Manifest Access ---
    godot::Ref<TaskManifest> get_active_manifest() const { return active_manifest; }
    int get_manifest_version() const;

    // --- Dynamic Creation for Manual Tasks ---
    [[nodiscard]] std::unique_ptr<INativeTask> create(const godot::StringName& p_name);
    godot::HashMap<godot::StringName, TaskUIFactories>* get_ui_factories() { return &ui_factories; }

    // --- Variadic Registration ---
    template <typename T_Task, typename T_Resource, typename T_Node, typename... Args>
    void register_task(const godot::StringName& p_name, Args... args) {
        // 1. Register Execution Strategy (Memory)
        manual_factories[p_name] = [args...]() -> std::unique_ptr<INativeTask> {
            return std::make_unique<T_Task>(args...);
        };

        // Capture type-safe instantiation closures (Memory)
        ui_factories[p_name] = {
            []() -> godot::Ref<godot_ext::TaskResource> {
                godot::Ref<T_Resource> res;
                res.instantiate();
                return res;
            },
            []() -> godot_ext::TaskGraphNode* {
                return memnew(T_Node); 
            }
        };

        // 2. Register UI Presentation Data (Cached until bake)
        godot::Dictionary task_def;
        task_def["resource_class"] = T_Resource::get_class_static();
        task_def["node_class"] = T_Node::get_class_static();
        pending_utility_matrix[p_name] = task_def;
    }

    // --- Legacy Godot Accessors (Routes to Singleton) ---
    static godot::Dictionary get_ui_query_matrix();
    static godot::Dictionary get_ui_transform_matrix();
    static godot::Dictionary get_ui_metadata_matrix();
    static godot::Dictionary get_ui_utility_matrix();
};

} // namespace ideam::core