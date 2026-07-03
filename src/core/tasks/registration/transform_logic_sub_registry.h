#pragma once

#include "transform_task_registry.h"
#include "task_view_bridge.h" // Ensures strict one-way dependency graph
#include "../transform_task.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

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
#include "../../memory/views/stencil_math.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"

namespace ideam::core {

namespace {
    // --- Resolvers & Extractor Logic ---
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
    template <typename T_Resolver> struct KernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::Type::KERNEL_POINTS)>> { static constexpr size_t value = T_Resolver::Type::KERNEL_POINTS; static constexpr bool has_kernel = true; };

    template <TransformLogicID ID, typename T_Concrete, typename T_Strategy> struct TransformLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BoundaryConstraint, C, S> { using Type = BoundaryConstraintTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BoundsExtraction, C, S> { using Type = BoundsExtractionTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::DataScatter, C, S> { using Type = DataScatterTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::DataSort, C, S> { using Type = DataSortTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::EulerIntegration, C, S> { using Type = EulerIntegrationTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::FastNoiseLite, C, S> { using Type = FastNoiseLiteTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::NoiseInjection, C, S> { using Type = NoiseInjectionTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R1, C, S> { 
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 1>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>; 
        static constexpr bool is_valid = true; 
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R2, C, S> { 
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 2>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>; 
        static constexpr bool is_valid = true; 
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R3, C, S> { 
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 3>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>; 
        static constexpr bool is_valid = true; 
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R1, C, S> { 
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 1>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>; 
        static constexpr bool is_valid = true; 
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R2, C, S> { 
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 2>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>; 
        static constexpr bool is_valid = true; 
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R3, C, S> { 
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 3>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>; 
        static constexpr bool is_valid = true; 
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ValueAccumulation, C, S> { using Type = ValueAccumulationTransformLogic<C>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, typename C, typename S> struct LogicKernelExtractor : KernelExtractorImpl<TransformLogicResolver<LogicID, C, S>> {};

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

    template <MemoryView ViewID, TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy> struct TransformViewResolver { static constexpr bool is_valid = false; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = StrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = LogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
}

template <TransformLogicID L>
struct TransformLogicSubRegistry {
    static std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE> factories;

    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing() {
        _fill_matrix<false>(std::make_index_sequence<TransformTaskRegistry::SUB_MATRIX_SIZE>{}, nullptr);
    }

    static void cleanup_execution_routing() {
        factories.fill(nullptr);
    }

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI
    static void generate_ui_matrices(godot::Dictionary& p_matrix) {
        IdeamTaskRegistry::log_with_registry(godot::vformat("    -> Evaluating LogicID: %d", static_cast<int64_t>(L)));
        
        _fill_matrix<true>(std::make_index_sequence<TransformTaskRegistry::SUB_MATRIX_SIZE>{}, &p_matrix);

        // POST-EVALUATION METRIC GATHERING
        godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));
        if (p_matrix.has(logic_key)) {
            godot::Dictionary dict = p_matrix[logic_key];
            godot::PackedInt64Array combos = dict["valid_combinations"];
            IdeamTaskRegistry::log_with_registry(godot::vformat("      -> SUCCESS: LogicID %d appended with %d valid combinations.", static_cast<int64_t>(L), combos.size()));
        } else {
            IdeamTaskRegistry::log_with_registry(godot::vformat("      -> FAILURE: LogicID %d resulted in 0 valid combinations. Key not created.", static_cast<int64_t>(L)));
        }
    }

private:
    template <bool BuildUI, size_t... Indices>
    static void _fill_matrix(std::index_sequence<Indices...>, godot::Dictionary* p_matrix) {
        (_fill_single<BuildUI, Indices>(p_matrix), ...);
    }

    template <bool BuildUI, size_t FlatIdx>
    static void _fill_single(godot::Dictionary* p_matrix) {
        constexpr size_t T_COUNT = TransformTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = TransformTaskRegistry::S_COUNT;

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

        using ResolvedView = TransformViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedView::is_valid) return;
        using V_Type = typename ResolvedView::Type;
        using V_Traits = ViewTraits<V_Type>;

        // --- FAST-FAIL TIER 2: Deep DOD Intersection ---
        constexpr bool is_fully_valid = []() consteval {
            // 1. Does the View support this Strategy at all?
            constexpr ViewStrategies iter_strategy_mask = to_view_strategy_mask(StrategyEnum);
            if ((V_Traits::supported_strategies & iter_strategy_mask) == ViewStrategies::NONE) return false;

            // 2. Does the primitive type satisfy both the View and the Logic?
            constexpr DataType iter_type_mask = Traits::DataFlag;
            if ((L_Type::required_types & iter_type_mask) == DataType::NONE) return false;
            if ((V_Traits::supported_types & iter_type_mask) == DataType::NONE) return false;

            // 3. Deep DOD Validation via Types (Capabilities, Layouts, Types, Dimensions, Kernels)
            return TransformLogicValidator::validate<L_Type, V_Type>();
        }();

        // --- FACTORY GENERATION ---
        if constexpr (is_fully_valid) {
            if constexpr (!BuildUI) {
                // Hot-Path Execution Routing
                factories[FlatIdx] = []() -> INativeTask* {
                    return new TransformTask<L_Type, V_Type, T_Strategy>();
                };
            } else {
                // Cold-Path UI Collection
                if (p_matrix) {

                    IdeamTaskRegistry::log_with_registry(
                        godot::vformat("        -> Valid Combo: T=%d, S=%d, V=%d(FlatIdx: %d)", T, S, V, FlatIdx)
                    );

                    godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));
                    
                    if (!p_matrix->has(logic_key)) {
                        godot::Dictionary dict;
                        // Instantiate the property array exactly once per logic struct type
                        dict["properties"] = L_Type::get_ui_properties();
                        dict["valid_combinations"] = godot::PackedInt64Array();
                        dict["name"] = godot::String(L_Type::display_name.data()); 
                        (*p_matrix)[logic_key] = dict;
                    }

                    // Map this valid 3D configuration hash (FlatIdx) so the UI knows it's an allowed permutation
                    godot::Dictionary dict = (*p_matrix)[logic_key];
                    godot::PackedInt64Array combos = dict["valid_combinations"];
                    combos.push_back(static_cast<int64_t>(FlatIdx));
                    dict["valid_combinations"] = combos; 
                    (*p_matrix)[logic_key] = dict; 
                }
            }
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