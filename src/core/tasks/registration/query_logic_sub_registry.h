#pragma once

#include "query_task_registry.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

// --- Logics ---
#include "../query_logic/aabb_query_logic.h"
#include "../query_logic/archetype_query_logic.h"
#include "../query_logic/bitmask_query_logic.h"
#include "../query_logic/boolean_query_logic.h"
#include "../query_logic/border_query_logic.h"
#include "../query_logic/color_query_logic.h"
#include "../query_logic/component_query_logic.h"
#include "../query_logic/data_comparison_query_logic.h"
#include "../query_logic/data_range_query_logic.h"
#include "../query_logic/directional_query_logic.h"
#include "../query_logic/distance_query_logic.h"
#include "../query_logic/event_ring_bridge_query_logic.h"
#include "../query_logic/frustum_query_logic.h"
#include "../query_logic/hierarchical_bridge_query_logic.h"
#include "../query_logic/limit_query_logic.h"
#include "../query_logic/morphological_query_logic.h"
#include "../query_logic/paged_to_tiled_bridge_query_logic.h"
#include "../query_logic/predicate_query_logic.h"
#include "../query_logic/relational_bridge_query_logic.h"
#include "../query_logic/spatial_inclusion_bridge_query_logic.h"
#include "../query_logic/spatial_projection_bridge_query_logic.h"
#include "../query_logic/stencil_dilation_bridge_query_logic.h"
#include "../query_logic/stochastic_query_logic.h"
#include "../query_logic/swap_eruption_bridge_query_logic.h"

// --- Views & Strategies ---
#include "../../memory/views/aosoa_view.h"
#include "../../memory/views/atomic_view.h"
#include "../../memory/views/bridge_view.h"
#include "../../memory/views/multi_element_view.h"
#include "../../memory/views/paged_view.h"
#include "../../memory/views/ring_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/sparse_set_view.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"

namespace ideam::core {

namespace {
    // --- Resolvers (Moved from Registry) ---
    consteval size_t floor_power_of_2(size_t n) {
        if (n == 0) return 1;
        size_t res = 1;
        while ((res << 1) <= n) res <<= 1;
        return res;
    }

    template <MemoryTypes MemType, BufferAlignmentMode AlignMode, size_t TargetVectorWidthBytes>
    struct AOSOALaneCalculator {
        static constexpr DataType DataFlag = NativeMemoryTraits<MemType>::DataFlag;
        static constexpr size_t get_lane_width() {
            if constexpr (DataFlag == DataType::CUSTOM) return 4;
            else {
                constexpr size_t byte_size = MemoryUtilities::get_type_byte_size(DataFlag, AlignMode);
                if constexpr (byte_size == 0) return 1;
                constexpr size_t raw_lanes = TargetVectorWidthBytes / byte_size;
                return floor_power_of_2(raw_lanes > 0 ? raw_lanes : 1);
            }
        }
    };

    template <typename S, typename = void> struct StrategyDimExtractor { static constexpr size_t value = 1; };
    template <typename S> struct StrategyDimExtractor<S, std::void_t<decltype(S::dimensions)>> { static constexpr size_t value = S::dimensions; };

    template <typename T_Resolver, typename Enable = void> struct KernelExtractorImpl { static constexpr size_t value = 0; static constexpr bool has_kernel = false; };
    template <typename T_Resolver> struct KernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::KernelSize)>> { static constexpr size_t value = T_Resolver::KernelSize; static constexpr bool has_kernel = true; };

    template <QueryLogicID ID, typename T_Concrete, typename T_Strategy> struct QueryLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::AABB, C, S> { using Type = AABBQueryLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Archetype, C, S> { using Type = ArchetypeQueryLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Bitmask, C, S> { using Type = BitmaskQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Boolean, C, S> { using Type = BooleanQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Border, C, S> { using Type = BorderQueryLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Color, C, S> { using Type = ColorQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Component, C, S> { using Type = ComponentQueryLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::DataComparison, C, S> { using Type = DataComparisonQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::DataRange, C, S> { using Type = DataRangeQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Directional, C, S> { using Type = DirectionalQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Distance, C, S> { using Type = DistanceQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::EventRingBridge, C, S> { using Type = EventRingBridgeQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Frustum, C, S> { using Type = FrustumQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::HierarchicalBridge, C, S> { using Type = HierarchicalBridgeQueryLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Limit, C, S> { using Type = LimitQueryLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Morphological, C, S> { using Type = MorphologicalQueryLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::PagedToTiledBridge, C, S> { using Type = PagedToTiledBridgeQueryLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Predicate, C, S> { using Type = PredicateQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::RelationalBridge, C, S> { using Type = RelationalBridgeQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::SpatialInclusionBridge, C, S> { using Type = SpatialInclusionBridgeQueryLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::SpatialProjectionBridge, C, S> { using Type = SpatialProjectionBridgeQueryLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::StencilDilationBridge, C, S> { using Type = StencilDilationBridgeQueryLogic<C, S, StrategyDimExtractor<S>::value>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::Stochastic, C, S> { using Type = StochasticQueryLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct QueryLogicResolver<QueryLogicID::SwapEruptionBridge, C, S> { using Type = SwapEruptionBridgeQueryLogic<C>; static constexpr bool is_valid = true; };

    template <QueryLogicID LogicID, typename C, typename S> struct LogicKernelExtractor : KernelExtractorImpl<QueryLogicResolver<LogicID, C, S>> {};

    template <MemoryStrategy ID> struct StrategyResolver { static constexpr bool is_valid = false; };
    template <> struct StrategyResolver<MemoryStrategy::FlatStrategy> { using Type = FlatStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::SoAStrategy> { using Type = SoAStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::AoSStrategy> { using Type = AoSStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial2DStrategy> { using Type = Spatial2DStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial3DStrategy> { using Type = Spatial3DStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial4DStrategy> { using Type = Spatial4DStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::TiledSoAStrategy> { using Type = TiledSoAStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::RingStrategy> { using Type = RingStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::PagedStrategy> { using Type = PagedStrategy; static constexpr bool is_valid = true; };

    template <MemoryView ViewID, QueryLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy> struct QueryViewResolver { static constexpr bool is_valid = false; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = StrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = LogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <QueryLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct QueryViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };

    template <typename T_Array> inline void _append_unique(T_Array& p_array, const godot::String& p_val) { if (!p_array.has(p_val)) p_array.push_back(p_val); }
}

template <QueryLogicID L>
struct QueryLogicSubRegistry {
    static std::array<QueryTaskFactoryFn, QueryTaskRegistry::SUB_MATRIX_SIZE> factories;

    static void init() {
        _fill_matrix(std::make_index_sequence<QueryTaskRegistry::SUB_MATRIX_SIZE>{});
    }

    static void cleanup() {
        factories.fill(nullptr);
    }

private:
    template <size_t... Indices>
    static void _fill_matrix(std::index_sequence<Indices...>) {
        (_fill_single<Indices>(), ...);
    }

    template <size_t FlatIdx>
    static void _fill_single() {
        constexpr size_t T_COUNT = QueryTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = QueryTaskRegistry::S_COUNT;
        constexpr size_t V_COUNT = QueryTaskRegistry::V_COUNT;

        // Decode the flat index back into 4D coordinates
        constexpr size_t T = FlatIdx % T_COUNT;
        constexpr size_t S = (FlatIdx / T_COUNT) % S_COUNT;
        constexpr size_t V = (FlatIdx / (T_COUNT * S_COUNT)) % V_COUNT;
        constexpr size_t O = FlatIdx / (T_COUNT * S_COUNT * V_COUNT);

        constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
        constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
        constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);
        constexpr QueryOp OpEnum = (O == 0) ? QueryOp::CULL : QueryOp::ADD;

        using Traits = NativeMemoryTraits<MemTypeEnum>;
        using ConcreteType = typename Traits::ConcreteType;
        using ResolvedStrategy = StrategyResolver<StrategyEnum>;
        using T_Strategy = typename ResolvedStrategy::Type;
        using ResolvedLogic = QueryLogicResolver<L, ConcreteType, T_Strategy>;
        using ResolvedView = QueryViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;

        constexpr bool combo_valid = ResolvedStrategy::is_valid && ResolvedLogic::is_valid && ResolvedView::is_valid;

        constexpr bool is_fully_valid = []() consteval {
            if constexpr (!combo_valid) return false;
            else {
                constexpr bool is_type_supported = (static_cast<uint64_t>(ResolvedLogic::Type::supported_types) & static_cast<uint64_t>(Traits::DataFlag)) != 0;
                constexpr bool is_op_supported = (OpEnum == QueryOp::CULL) ? ResolvedLogic::Type::supports_cull : ResolvedLogic::Type::supports_addition;
                
                if constexpr (!is_type_supported || !is_op_supported) return false;
                else {
                    return QueryLogicValidator::validate(
                        ResolvedLogic::Type::requirements, 
                        ResolvedLogic::Type::supported_layouts, 
                        ViewTraits<typename ResolvedView::Type>::capabilities, 
                        BufferLayoutType::NONE
                    );
                }
            }
        }();

        if constexpr (is_fully_valid) {
            factories[FlatIdx] = []() -> INativeTask* {
                return new QueryTask<typename ResolvedLogic::Type, OpEnum, typename ResolvedView::Type, T_Strategy>();
            };

            /*
            // UI Dictionary block temporarily commented out to fix missing `type_name` MSVC errors
            if constexpr (O == 0) {
                if (QueryTaskRegistry::ui_query_matrix) {
                    godot::String logic_name(ResolvedLogic::Type::type_name);
                    if (!QueryTaskRegistry::ui_query_matrix->has(logic_name)) {
                        godot::Dictionary dict;
                        dict["ops"] = godot::Array(); dict["views"] = godot::Array(); dict["strategies"] = godot::Array();
                        (*QueryTaskRegistry::ui_query_matrix)[logic_name] = dict;
                    }
                    godot::Dictionary dict = (*QueryTaskRegistry::ui_query_matrix)[logic_name];
                    _append_unique<godot::Array>(dict["views"], ResolvedView::Type::type_name);
                    _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
                }
            }
            if (QueryTaskRegistry::ui_query_matrix) {
                godot::Dictionary dict = (*QueryTaskRegistry::ui_query_matrix)[ResolvedLogic::Type::type_name];
                _append_unique<godot::Array>(dict["ops"], (O == 0) ? "CULL" : "ADD");
            }
            */
        }
    }
};

template <QueryLogicID L>
std::array<QueryTaskFactoryFn, QueryTaskRegistry::SUB_MATRIX_SIZE> 
QueryLogicSubRegistry<L>::factories = []{
    std::array<QueryTaskFactoryFn, QueryTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core