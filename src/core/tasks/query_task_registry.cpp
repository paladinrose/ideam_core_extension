#include "query_task_registry.h"
#include "query_task.h"

// --- Logics ---
#include "query_logic/aabb_query_logic.h"
#include "query_logic/archetype_query_logic.h"
#include "query_logic/bitmask_query_logic.h"
#include "query_logic/boolean_query_logic.h"
#include "query_logic/border_query_logic.h"
#include "query_logic/color_query_logic.h"
#include "query_logic/component_query_logic.h"
#include "query_logic/data_comparison_query_logic.h"
#include "query_logic/data_range_query_logic.h"
#include "query_logic/directional_query_logic.h"
#include "query_logic/distance_query_logic.h"
#include "query_logic/event_ring_bridge_query_logic.h"
#include "query_logic/frustum_query_logic.h"
#include "query_logic/hierarchical_bridge_query_logic.h"
#include "query_logic/limit_query_logic.h"
#include "query_logic/morphological_query_logic.h"
#include "query_logic/paged_to_tiled_bridge_query_logic.h"
#include "query_logic/predicate_query_logic.h"
#include "query_logic/relational_bridge_query_logic.h"
#include "query_logic/spatial_inclusion_bridge_query_logic.h"
#include "query_logic/spatial_projection_bridge_query_logic.h"
#include "query_logic/stencil_dilation_bridge_query_logic.h"
#include "query_logic/stochastic_query_logic.h"
#include "query_logic/swap_eruption_bridge_query_logic.h"

// --- Views & Strategies ---
#include "../memory/views/single_element_view.h"
#include "../memory/views/strategies.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/array.hpp>
#include <tuple>

namespace ideam::core {

// ============================================================================
// THE TRANSLATION UNIT FIREWALL (4D Metaprogramming)
// ============================================================================
namespace { 
    // Note: Assuming primitive baselines (float, Vector3) based on standard traits.
    // Adjust template parameters if a specific query enforces a different type structure.
    using Logics = std::tuple<
        AABBQueryLogic,
        ArchetypeQueryLogic,
        BitmaskQueryLogic,
        BooleanQueryLogic,
        BorderQueryLogic<godot::Vector3>,
        ColorQueryLogic,
        ComponentQueryLogic,
        DataComparisonQueryLogic<float>,
        DataRangeQueryLogic<float>,
        DirectionalQueryLogic<godot::Vector3>,
        DistanceQueryLogic<godot::Vector3>,
        EventRingBridgeQueryLogic,
        FrustumQueryLogic<godot::Vector3>,
        HierarchicalBridgeQueryLogic,
        LimitQueryLogic,
        MorphologicalQueryLogic,
        PagedToTiledBridgeQueryLogic,
        PredicateQueryLogic<float>,
        RelationalBridgeQueryLogic,
        SpatialInclusionBridgeQueryLogic<godot::Vector3>,
        SpatialProjectionBridgeQueryLogic<godot::Vector3>,
        StencilDilationBridgeQueryLogic,
        StochasticQueryLogic,
        SwapEruptionBridgeQueryLogic
    >;

    using Views = std::tuple<
        SingleElementView<float, FlatStrategy>
    >;
    
    using Strats = std::tuple<
        FlatStrategy, 
        Spatial2DStrategy, 
        Spatial3DStrategy
    >;

    // Factory Template: Validates BOTH the Layout/View support AND the QueryOp support
    template<typename T_Logic, QueryOp Op, typename T_View, typename T_Strategy>
    INativeTask* create_query_task() {
        // Compile-time Op check: Does this logic actually support CULL or ADD?
        if constexpr ((Op == QueryOp::CULL && T_Logic::supports_cull) ||
                      (Op == QueryOp::ADD && T_Logic::supports_addition)) {
            
            // Compile-time Layout check: Does the view meet the logic's hardware requirements?
            if constexpr (QueryLogicValidator::validate(
                T_Logic::requirements, 
                T_Logic::supported_layouts, 
                ViewTraits<T_View>::capabilities, 
                BufferLayoutType::NONE)) 
            {
                return new QueryTask<T_Logic, Op, T_View, T_Strategy>(T_Logic{});
            }
        }
        return nullptr;
    }

    // Recursive 4D Metaprogramming filler
    template<size_t L, size_t O, size_t V, size_t S>
    struct QueryFactoryMatrix {
        static void fill(std::array<QueryTaskFactoryFn, QueryTaskRegistry::TOTAL_COMBINATIONS>& arr) {
            
            // 4D array flattened to 1D index
            size_t idx = (L * QueryTaskRegistry::O_COUNT * QueryTaskRegistry::V_COUNT * QueryTaskRegistry::S_COUNT) + 
                         (O * QueryTaskRegistry::V_COUNT * QueryTaskRegistry::S_COUNT) +
                         (V * QueryTaskRegistry::S_COUNT) + S;
            
            constexpr QueryOp op_val = (O == 0) ? QueryOp::CULL : QueryOp::ADD;

            arr[idx] = create_query_task<
                std::tuple_element_t<L, Logics>, 
                op_val,
                std::tuple_element_t<V, Views>, 
                std::tuple_element_t<S, Strats>
            >();

            // Recursive stepping (Inner-most loop to outer-most loop)
            if constexpr (S + 1 < QueryTaskRegistry::S_COUNT) {
                fill<L, O, V, S + 1>(arr);
            } else if constexpr (V + 1 < QueryTaskRegistry::V_COUNT) {
                fill<L, O, V + 1, 0>(arr);
            } else if constexpr (O + 1 < QueryTaskRegistry::O_COUNT) {
                fill<L, O + 1, 0, 0>(arr);
            } else if constexpr (L + 1 < QueryTaskRegistry::L_COUNT) {
                fill<L + 1, 0, 0, 0>(arr);
            }
        }
    };
} // end anonymous namespace

// Global static initialization
const std::array<QueryTaskFactoryFn, QueryTaskRegistry::TOTAL_COMBINATIONS> 
QueryTaskRegistry::factories = []{
    std::array<QueryTaskFactoryFn, QueryTaskRegistry::TOTAL_COMBINATIONS> arr;
    arr.fill(nullptr);
    QueryFactoryMatrix<0, 0, 0, 0>::fill(arr);
    return arr;
}();

godot::Dictionary* QueryTaskRegistry::ui_query_matrix = nullptr;

void QueryTaskRegistry::init() {
    ui_query_matrix = new godot::Dictionary();
    // Population logic identical to Metadata/Transform registries
}

void QueryTaskRegistry::cleanup() {
    delete ui_query_matrix;
    ui_query_matrix = nullptr;
}

std::unique_ptr<INativeTask> QueryTaskRegistry::create(uint32_t p_logic_id, uint32_t p_op_id, uint32_t p_view_id, uint32_t p_strategy_id) {
    size_t flat_idx = (p_logic_id * O_COUNT * V_COUNT * S_COUNT) + 
                      (p_op_id * V_COUNT * S_COUNT) + 
                      (p_view_id * S_COUNT) + p_strategy_id;
    
    if (flat_idx < TOTAL_COMBINATIONS && factories[flat_idx] != nullptr) {
        return std::unique_ptr<INativeTask>(factories[flat_idx]());
    }
    
    return nullptr;
}

} // namespace ideam::core