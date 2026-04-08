#ifndef IDEAM_CORE_NATIVE_TASK_REGISTRY_H
#define IDEAM_CORE_NATIVE_TASK_REGISTRY_H

#include "i_native_task.h"
#include "test_task.h"
#include "query_task.h"
#include "metadata_task.h"
#include "transform_task.h"
#include "../simulations/simulation_task.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>
#include <functional>
#include <tuple>

namespace ideam::core {

using NativeTaskFactory = std::function<std::unique_ptr<INativeTask>()>;

class NativeTaskRegistry {
private:
    static godot::HashMap<godot::StringName, NativeTaskFactory> factories;

    // --- Isolated UI Matrices ---
    static godot::Dictionary ui_query_matrix;
    static godot::Dictionary ui_transform_matrix;
    static godot::Dictionary ui_metadata_matrix;
    static godot::Dictionary ui_simulation_matrix;

    template <typename T>
    static void _append_unique(godot::Array& r_array, const godot::String& p_val) {
        if (!r_array.has(p_val)) r_array.push_back(p_val);
    }

public:
    static std::unique_ptr<INativeTask> create(const godot::StringName& p_name);
    static void cleanup();

    // UI Dictionary Accessors
    static godot::Dictionary get_ui_query_matrix() { return ui_query_matrix; }
    static godot::Dictionary get_ui_transform_matrix() { return ui_transform_matrix; }
    static godot::Dictionary get_ui_metadata_matrix() { return ui_metadata_matrix; }
    static godot::Dictionary get_ui_simulation_matrix() { return ui_simulation_matrix; }

    // --- 1. Base Registration (For TestTask and manual overrides) ---
    template <typename T_Task>
    static void register_task(const godot::StringName& p_name) {
        factories[p_name] = []() -> std::unique_ptr<INativeTask> {
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
            
            factories[task_name] = []() { return std::make_unique<QueryTask<T_Logic, Op, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_query_matrix.has(logic_name)) {
                godot::Dictionary dict;
                dict["ops"] = godot::Array(); dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                ui_query_matrix[logic_name] = dict;
            }
            godot::Dictionary dict = ui_query_matrix[logic_name];
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
            factories[task_name] = []() { return std::make_unique<TransformTask<T_Logic, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_transform_matrix.has(logic_name)) {
                godot::Dictionary dict;
                dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                ui_transform_matrix[logic_name] = dict;
            }
            godot::Dictionary dict = ui_transform_matrix[logic_name];
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
            factories[task_name] = []() { return std::make_unique<MetadataTask<T_Logic, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_metadata_matrix.has(logic_name)) {
                godot::Dictionary dict;
                dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                ui_metadata_matrix[logic_name] = dict;
            }
            godot::Dictionary dict = ui_metadata_matrix[logic_name];
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
            factories[task_name] = []() { return std::make_unique<SimulationTask<T_Logic, T_View, T_Strategy>>(); };

            godot::String logic_name(T_Logic::type_name);
            if (!ui_simulation_matrix.has(logic_name)) {
                godot::Dictionary dict;
                dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                ui_simulation_matrix[logic_name] = dict;
            }
            godot::Dictionary dict = ui_simulation_matrix[logic_name];
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