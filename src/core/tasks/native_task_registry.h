#ifndef IDEAM_CORE_NATIVE_TASK_REGISTRY_H
#define IDEAM_CORE_NATIVE_TASK_REGISTRY_H

#include "i_native_task.h"
#include "entry_fill_task.h"
#include "query_task.h"
#include "metadata_task.h"
#include "transform_task.h"
#include "../simulations/simulation_task.h"

// --- Logic ---
#include "query_logic/stochastic_query_logic.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>
#include <functional>
#include <tuple>

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

template <> struct NativeMemoryTraits<MemoryTypes::BOOL> {
    using ConcreteType = bool;
    static constexpr DataType DataFlag = DataType::BOOL;
};

template <> struct NativeMemoryTraits<MemoryTypes::BYTE> {
    using ConcreteType = uint8_t;
    static constexpr DataType DataFlag = DataType::BYTE;
};

template <> struct NativeMemoryTraits<MemoryTypes::INT32> {
    using ConcreteType = int32_t;
    static constexpr DataType DataFlag = DataType::INT32;
};

template <> struct NativeMemoryTraits<MemoryTypes::INT64> {
    using ConcreteType = int64_t;
    static constexpr DataType DataFlag = DataType::INT64;
};

template <> struct NativeMemoryTraits<MemoryTypes::FLOAT32> {
    using ConcreteType = float;
    static constexpr DataType DataFlag = DataType::FLOAT32;
};

template <> struct NativeMemoryTraits<MemoryTypes::FLOAT64> {
    using ConcreteType = double;
    static constexpr DataType DataFlag = DataType::FLOAT64;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2> {
    using ConcreteType = godot::Vector2;
    static constexpr DataType DataFlag = DataType::VECTOR2;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3> {
    using ConcreteType = godot::Vector3;
    static constexpr DataType DataFlag = DataType::VECTOR3;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4> {
    using ConcreteType = godot::Vector4;
    static constexpr DataType DataFlag = DataType::VECTOR4;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2I> {
    using ConcreteType = godot::Vector2i;
    static constexpr DataType DataFlag = DataType::VECTOR2I;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3I> {
    using ConcreteType = godot::Vector3i;
    static constexpr DataType DataFlag = DataType::VECTOR3I;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4I> {
    using ConcreteType = godot::Vector4i;
    static constexpr DataType DataFlag = DataType::VECTOR4I;
};

// Note: Mapped to Godot's standard Vector classes; precision handled by Godot's build flags
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2D> {
    using ConcreteType = godot::Vector2; 
    static constexpr DataType DataFlag = DataType::VECTOR2D;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3D> {
    using ConcreteType = godot::Vector3; 
    static constexpr DataType DataFlag = DataType::VECTOR3D;
};

template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4D> {
    using ConcreteType = godot::Vector4; 
    static constexpr DataType DataFlag = DataType::VECTOR4D;
};

template <> struct NativeMemoryTraits<MemoryTypes::COLOR> {
    using ConcreteType = godot::Color;
    static constexpr DataType DataFlag = DataType::COLOR;
};

template <> struct NativeMemoryTraits<MemoryTypes::CUSTOM> {
    using ConcreteType = void*; // Placeholder for custom logic handling
    static constexpr DataType DataFlag = DataType::CUSTOM;
};

using NativeTaskFactory = std::function<std::unique_ptr<INativeTask>()>;

class NativeTaskRegistry {
private:
    static godot::HashMap<godot::StringName, NativeTaskFactory>* factories;

    // --- Isolated UI Matrices ---
    static godot::Dictionary* ui_query_matrix;
    static godot::Dictionary* ui_transform_matrix;
    static godot::Dictionary* ui_metadata_matrix;
    static godot::Dictionary* ui_simulation_matrix;

    template <typename T>
    static void _append_unique(godot::Array& r_array, const godot::String& p_val) {
        if (!r_array.has(p_val)) r_array.push_back(p_val);
    }

public:
    static void init();
    static std::unique_ptr<INativeTask> create(const godot::StringName& p_name);
    static void cleanup();

    // UI Dictionary Accessors
    static godot::Dictionary get_ui_query_matrix() { return *ui_query_matrix; }
    static godot::Dictionary get_ui_transform_matrix() { return *ui_transform_matrix; }
    static godot::Dictionary get_ui_metadata_matrix() { return *ui_metadata_matrix; }
    static godot::Dictionary get_ui_simulation_matrix() { return *ui_simulation_matrix; }

    // --- 1. Base Registration (For TestTask and manual overrides) ---
    template <typename T_Task>
    static void register_task(const godot::StringName& p_name) {
        (*factories)[p_name] = []() -> std::unique_ptr<INativeTask> {
            return std::make_unique<T_Task>();
        };
    }

    // --- 2. QueryTask Matrix Builder (Requires QueryOp) ---
    template <typename T_Logic, typename T_Op, typename T_View, typename T_Strategy>
    static void try_register_query() {
        constexpr QueryOp Op = T_Op::value;
        if constexpr (
            ((Op == QueryOp::CULL && T_Logic::supports_cull) || (Op == QueryOp::ADD && T_Logic::supports_addition)) &&
            QueryLogicValidator::validate(T_Logic::requirements, T_Logic::supported_layouts, ViewTraits<T_View>::capabilities, BufferLayoutType::NONE)
        ) {
            godot::String op_str = (Op == QueryOp::CULL) ? "CULL" : "ADD";
            godot::String task_name = godot::String("Query_") + T_Logic::type_name + "_" + op_str + "_" + T_View::type_name + "_" + T_Strategy::type_name;
            
            (*factories)[task_name] = []() { return std::make_unique<QueryTask<T_Logic, Op, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_query_matrix->has(logic_name)) {
                godot::Dictionary dict;
                dict["ops"] = godot::Array(); dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                (*ui_query_matrix)[logic_name] = dict;
            }
            godot::Dictionary dict = (*ui_query_matrix)[logic_name];
            _append_unique<godot::Array>(dict["ops"], op_str);
            _append_unique<godot::Array>(dict["views"], T_View::type_name);
            _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
        }
    }

    template <typename TupleLogics, typename TupleOps, typename TupleViews, typename TupleStrats>
    struct QueryMatrixBuilder {
        template <typename L, typename O, typename V> static void iterate_strats() { std::apply([]<typename... Ss>(Ss...) { (try_register_query<L, O, V, Ss>(), ...); }, TupleStrats{}); }
        template <typename L, typename O> static void iterate_views() { std::apply([]<typename... Vs>(Vs...) { (iterate_strats<L, O, Vs>(), ...); }, TupleViews{}); }
        template <typename L> static void iterate_ops() { std::apply([]<typename... Os>(Os...) { (iterate_views<L, Os>(), ...); }, TupleOps{}); }
        static void build() { std::apply([]<typename... Ls>(Ls...) { (iterate_ops<Ls>(), ...); }, TupleLogics{}); }
    };

    // --- 3. TransformTask Matrix Builder ---
    template <typename T_Logic, typename T_View, typename T_Strategy>
    static void try_register_transform() {
        if constexpr (TransformLogicValidator::validate(T_Logic::requirements, T_Logic::supported_layouts, ViewTraits<T_View>::capabilities, BufferLayoutType::NONE)) {
            godot::String task_name = godot::String("Transform_") + T_Logic::type_name + "_" + T_View::type_name + "_" + T_Strategy::type_name;
            (*factories)[task_name] = []() { return std::make_unique<TransformTask<T_Logic, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_transform_matrix->has(logic_name)) {
                godot::Dictionary dict;
                dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                (*ui_transform_matrix)[logic_name] = dict;
            }
            godot::Dictionary dict = (*ui_transform_matrix)[logic_name];
            _append_unique<godot::Array>(dict["views"], T_View::type_name);
            _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
        }
    }

    template <typename TupleLogics, typename TupleViews, typename TupleStrats>
    struct TransformMatrixBuilder {
        template <typename L, typename V> static void iterate_strats() { std::apply([]<typename... Ss>(Ss...) { (try_register_transform<L, V, Ss>(), ...); }, TupleStrats{}); }
        template <typename L> static void iterate_views() { std::apply([]<typename... Vs>(Vs...) { (iterate_strats<L, Vs>(), ...); }, TupleViews{}); }
        static void build() { std::apply([]<typename... Ls>(Ls...) { (iterate_views<Ls>(), ...); }, TupleLogics{}); }
    };

    // --- 4. MetadataTask Matrix Builder ---
    template <typename T_Logic, typename T_View, typename T_Strategy>
    static void try_register_metadata() {
        if constexpr (MetadataLogicValidator::validate(T_Logic::requirements, T_Logic::supported_layouts, ViewTraits<T_View>::capabilities, BufferLayoutType::NONE)) {
            godot::String task_name = godot::String("Metadata_") + T_Logic::type_name + "_" + T_View::type_name + "_" + T_Strategy::type_name;
            (*factories)[task_name] = []() { return std::make_unique<MetadataTask<T_Logic, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_metadata_matrix->has(logic_name)) {
                godot::Dictionary dict;
                dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                (*ui_metadata_matrix)[logic_name] = dict;
            }
            godot::Dictionary dict = (*ui_metadata_matrix)[logic_name];
            _append_unique<godot::Array>(dict["views"], T_View::type_name);
            _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
        }
    }

    template <typename TupleLogics, typename TupleViews, typename TupleStrats>
    struct MetadataMatrixBuilder {
        template <typename L, typename V> static void iterate_strats() { std::apply([]<typename... Ss>(Ss...) { (try_register_metadata<L, V, Ss>(), ...); }, TupleStrats{}); }
        template <typename L> static void iterate_views() { std::apply([]<typename... Vs>(Vs...) { (iterate_strats<L, Vs>(), ...); }, TupleViews{}); }
        static void build() { std::apply([]<typename... Ls>(Ls...) { (iterate_views<Ls>(), ...); }, TupleLogics{}); }
    };

    // --- 5. SimulationTask Matrix Builder ---
    template <typename T_Logic, typename T_View, typename T_Strategy>
    static void try_register_simulation() {
        if constexpr (SimulationLogicValidator::validate(T_Logic::requirements, T_Logic::supported_layouts, ViewTraits<T_View>::capabilities, BufferLayoutType::NONE)) {
            godot::String task_name = godot::String("Simulation_") + T_Logic::type_name + "_" + T_View::type_name + "_" + T_Strategy::type_name;
            (*factories)[task_name] = []() { return std::make_unique<SimulationTask<T_Logic, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_simulation_matrix->has(logic_name)) {
                godot::Dictionary dict;
                dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                (*ui_simulation_matrix)[logic_name] = dict;
            }
            godot::Dictionary dict = (*ui_simulation_matrix)[logic_name];
            _append_unique<godot::Array>(dict["views"], T_View::type_name);
            _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
        }
    }

    template <typename TupleLogics, typename TupleViews, typename TupleStrats>
    struct SimulationMatrixBuilder {
        template <typename L, typename V> static void iterate_strats() { std::apply([]<typename... Ss>(Ss...) { (try_register_simulation<L, V, Ss>(), ...); }, TupleStrats{}); }
        template <typename L> static void iterate_views() { std::apply([]<typename... Vs>(Vs...) { (iterate_strats<L, Vs>(), ...); }, TupleViews{}); }
        static void build() { std::apply([]<typename... Ls>(Ls...) { (iterate_views<Ls>(), ...); }, TupleLogics{}); }
    };
};

} // namespace ideam::core

#endif // IDEAM_CORE_NATIVE_TASK_REGISTRY_H