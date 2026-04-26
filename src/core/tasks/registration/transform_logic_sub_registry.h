#pragma once

#include "transform_task_registry.h"
#include "task_view_bridge.h" // Ensures strict one-way dependency graph
#include "../transform_task.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

// --- Logics ---
#include "../transform_logic/boundary_constraint_transform_logic.h"
#include "../transform_logic/bounds_extraction_transform_logic.h"
#include "../transform_logic/data_scatter_transform_logic.h"
#include "../transform_logic/data_sort_transform_logic.h"
#include "../transform_logic/euler_integration_transform_logic.h"
#include "../transform_logic/fast_noise_lite_transform_logic.h"
#include "../transform_logic/noise_injection_transform_logic.h"
#include "../transform_logic/stencil_convolution_transform_logic.h"
#include "../transform_logic/value_accumulation_transform_logic.h"

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
    // --- Resolvers & Memory Core ---
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

    // --- Transform Logic Resolver ---
    // Maps the TransformLogicID enum pseudo-entries into concrete, fully specialized Logic types.
    template <TransformLogicID L, typename T, typename T_Strategy>
    struct TransformLogicResolver {
        static constexpr bool is_valid = false;
    };

    // --- Single Template Argument (<T>) ---
    
    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::BoundaryConstraint, T, T_Strategy> {
        using Type = BoundaryConstraintTransformLogic<T>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::BoundsExtraction, T, T_Strategy> {
        using Type = BoundsExtractionTransformLogic<T>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::DataScatter, T, T_Strategy> {
        using Type = DataScatterTransformLogic<T>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::DataSort, T, T_Strategy> {
        using Type = DataSortTransformLogic<T>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::NoiseInjection, T, T_Strategy> {
        using Type = NoiseInjectionTransformLogic<T>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::ValueAccumulation, T, T_Strategy> {
        using Type = ValueAccumulationTransformLogic<T>; static constexpr bool is_valid = true;
    };

    // --- Concrete Types (No Template Arguments) ---

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::EulerIntegration, T, T_Strategy> {
        using Type = EulerIntegrationTransformLogic; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::FastNoiseLite, T, T_Strategy> {
        using Type = FastNoiseLiteTransformLogic; static constexpr bool is_valid = true;
    };

    // --- Stencil Pseudo-Variants (<T, T_Strategy, KernelSize>) ---

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R1, T, T_Strategy> {
        using Type = StencilConvolutionTransformLogic<T, T_Strategy, 9>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R2, T, T_Strategy> {
        using Type = StencilConvolutionTransformLogic<T, T_Strategy, 25>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R3, T, T_Strategy> {
        using Type = StencilConvolutionTransformLogic<T, T_Strategy, 49>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R1, T, T_Strategy> {
        using Type = StencilConvolutionTransformLogic<T, T_Strategy, 5>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R2, T, T_Strategy> {
        using Type = StencilConvolutionTransformLogic<T, T_Strategy, 13>; static constexpr bool is_valid = true;
    };

    template <typename T, typename T_Strategy>
    struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R3, T, T_Strategy> {
        using Type = StencilConvolutionTransformLogic<T, T_Strategy, 25>; static constexpr bool is_valid = true;
    };

    template <typename T_Resolver, typename Enable = void> struct KernelExtractorImpl { static constexpr size_t value = 0; static constexpr bool has_kernel = false; };
    template <typename T_Resolver> struct KernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::Type::KernelSize)>> { static constexpr size_t value = T_Resolver::Type::KernelSize; static constexpr bool has_kernel = true; };
    
    template <TransformLogicID LogicID, typename T_Concrete, typename T_Strategy> struct LogicKernelExtractor : KernelExtractorImpl<TransformLogicResolver<LogicID, T_Concrete, T_Strategy>> {};

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

    template <MemoryView ViewID, TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy> struct ViewResolver { static constexpr bool is_valid = false; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = S::dimensions; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = LogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct ViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };

    template <typename T_Array>
    inline void _append_unique(T_Array& p_array, const godot::String& p_val) {
        if (!p_array.has(p_val)) {
            p_array.push_back(p_val);
        }
    }
}

template <TransformLogicID L>
struct TransformLogicSubRegistry {
    static std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE> factories;

    static void init() {
        _fill_matrix(std::make_index_sequence<TransformTaskRegistry::SUB_MATRIX_SIZE>{});
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
        constexpr size_t T_COUNT = TransformTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = TransformTaskRegistry::S_COUNT;

        // Decode the flat index back into 3D coordinates
        constexpr size_t T = FlatIdx % T_COUNT;
        constexpr size_t S = (FlatIdx / T_COUNT) % S_COUNT;
        constexpr size_t V = FlatIdx / (T_COUNT * S_COUNT);

        constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
        constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
        constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

        using Traits = NativeMemoryTraits<MemTypeEnum>;
        using ConcreteType = typename Traits::ConcreteType;

        // --- FAST-FAIL TIER 1: Base Resolution ---
        using ResolvedStrategy = StrategyResolver<StrategyEnum>;
        if constexpr (!ResolvedStrategy::is_valid) return;
        using T_Strategy = typename ResolvedStrategy::Type;

        using ResolvedLogic = TransformLogicResolver<L, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedLogic::is_valid) return;
        using L_Type = typename ResolvedLogic::Type;

        using ResolvedView = ViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedView::is_valid) return;
        using V_Type = typename ResolvedView::Type;
        using V_Traits = ViewTraits<V_Type>;

        // --- FAST-FAIL TIER 2: Deep DOD Intersection ---
        constexpr bool is_fully_valid = []() consteval {
            // 1. Does the View support the Strategy?
            constexpr ViewStrategies iter_strategy_mask = to_view_strategy_mask(StrategyEnum);
            if ((V_Traits::supported_strategies & iter_strategy_mask) == ViewStrategies::NONE) return false;

            // 2. Does the Primitive DataType align across Logic and View requirements?
            constexpr DataType iter_type_mask = Traits::DataFlag;
            if ((L_Type::required_types & iter_type_mask) == DataType::NONE) return false;
            if ((V_Traits::supported_types & iter_type_mask) == DataType::NONE) return false;

            // 3. The Core 6-Argument DOD Validator
            return TransformLogicValidator::validate(
                L_Type::required_capabilities, 
                L_Type::required_layouts, 
                L_Type::required_types,
                V_Traits::capabilities, 
                V_Traits::supported_layouts, 
                V_Traits::supported_types
            );
        }();

        // --- FACTORY GENERATION ---
        if constexpr (is_fully_valid) {
            factories[FlatIdx] = []() -> INativeTask* {
                return new TransformTask<L_Type, V_Type, T_Strategy>();
            };

            /*
            // UI Dictionary block temporarily commented out to fix missing `type_name` MSVC errors
            if (TransformTaskRegistry::ui_transform_matrix) {
                godot::String logic_name(L_Type::type_name);
                if (!TransformTaskRegistry::ui_transform_matrix->has(logic_name)) {
                    godot::Dictionary dict;
                    dict["views"] = godot::Array(); 
                    dict["strategies"] = godot::Array();
                    (*TransformTaskRegistry::ui_transform_matrix)[logic_name] = dict;
                }
                godot::Dictionary dict = (*TransformTaskRegistry::ui_transform_matrix)[logic_name];
                _append_unique<godot::Array>(dict["views"], V_Type::type_name);
                _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
            }
            */
        }
    }
};

template <TransformLogicID L>
std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE> 
TransformLogicSubRegistry<L>::factories = []{
    std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core