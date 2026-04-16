// transform_task_registry.cpp
#include "transform_task_registry.h"
#include "transform_task.h"

// --- Logics ---
#include "transforms/boundary_constraint_transform_logic.h"
#include "transforms/bounds_extraction_transform_logic.h"
#include "transforms/data_scatter_transform_logic.h"
#include "transforms/data_sort_transform_logic.h"
#include "transforms/euler_integration_transform_logic.h"
#include "transforms/fast_noise_lite_transform_logic.h"
#include "transforms/noise_injection_transform_logic.h"
#include "transforms/stencil_convolution_transform_logic.h"
#include "transforms/value_accumulation_transform_logic.h"

// --- Views ---
#include "../memory/views/aosoa_view.h"
#include "../memory/views/atomic_view.h"
#include "../memory/views/bridge_view.h"
#include "../memory/views/multi_element_view.h"
#include "../memory/views/paged_view.h"
#include "../memory/views/ring_view.h"
#include "../memory/views/single_element_view.h"
#include "../memory/views/sparse_set_view.h"
#include "../memory/views/static_stencil_view.h"
#include "../memory/views/stencil_view.h"
#include "../memory/views/swap_view.h"

// --- Strategies ---
#include "../memory/views/strategies.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/array.hpp>
#include <tuple>

namespace ideam::core {

// 2. COMPILE-TIME RESOLVERS & CALCULATORS (Place in transform_task_registry.cpp anonymous namespace)

namespace {

    // --- AOSOA Lane Width Calculator ---
    consteval size_t floor_power_of_2(size_t n) {
        if (n == 0) return 1; // Failsafe to prevent zero-lane SIMD
        size_t res = 1;
        while ((res << 1) <= n) res <<= 1;
        return res;
    }

    template <MemoryTypes MemType, BufferAlignmentMode AlignMode, size_t TargetVectorWidthBytes>
    struct AOSOALaneCalculator {
        static constexpr DataType DataFlag = NativeMemoryTraits<MemType>::DataFlag;
        
        static constexpr size_t get_lane_width() {
            if constexpr (DataFlag == DataType::CUSTOM) {
                return 4; // Failsafe for Custom structs
            } else {
                constexpr size_t byte_size = MemoryUtilities::get_type_byte_size(DataFlag, AlignMode);
                if constexpr (byte_size == 0) return 1;
                constexpr size_t raw_lanes = TargetVectorWidthBytes / byte_size;
                return floor_power_of_2(raw_lanes > 0 ? raw_lanes : 1);
            }
        }
    };

    // --- Logic Kernel Size Extractor (SFINAE) ---
    // Safely retrieves KernelSize from Stencil Logics, and defaults non-stencils to 0
    template <TransformLogicID LogicID, typename T_Concrete, typename T_Strategy, typename Enable = void>
    struct LogicKernelExtractor {
        static constexpr size_t value = 0;
        static constexpr bool has_kernel = false;
    };

    // --- Logic Kernel Size Extractor (SFINAE) ---
    // Safely retrieves KernelSize from Stencil Logics, and defaults non-stencils to 0
    template <typename T_Resolver, typename Enable = void>
    struct KernelExtractorImpl {
        static constexpr size_t value = 0;
        static constexpr bool has_kernel = false;
    };

    template <typename T_Resolver>
    struct KernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::KernelSize)>> {
        static constexpr size_t value = T_Resolver::KernelSize;
        static constexpr bool has_kernel = true;
    };

    template <TransformLogicID LogicID, typename T_Concrete, typename T_Strategy>
    struct LogicKernelExtractor : KernelExtractorImpl<LogicResolver<LogicID, T_Concrete, T_Strategy>> {};

    // --- Strategy Resolver ---
    template <MemoryStrategy ID> struct StrategyResolver;
    template <> struct StrategyResolver<MemoryStrategy::FlatStrategy> { using Type = FlatStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::SoAStrategy> { using Type = SoAStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::AoSStrategy> { using Type = AoSStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial2DStrategy> { using Type = Spatial2DStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial3DStrategy> { using Type = Spatial3DStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial4DStrategy> { using Type = Spatial4DStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::TiledSoAStrategy> { using Type = TiledSoAStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::RingStrategy> { using Type = RingStrategy; };
    template <> struct StrategyResolver<MemoryStrategy::PagedStrategy> { using Type = PagedStrategy; };

    // --- View Resolver ---
    template <MemoryView ViewID, TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver;

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::SingleElementView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = SingleElementView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::MultiElementView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = MultiElementView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::SparseSetView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = SparseSetView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::PagedView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = PagedView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::RingView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = RingView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::StencilView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = StencilView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AtomicView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = AtomicView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::SwapView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Type = SwapView<T_Concrete, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::BridgeView, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t DimCount = T_Strategy::dimensions;
        using Type = BridgeView<T_Concrete, T_Concrete, DimCount, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::StaticStencilView, LogicID, MemType, T_Concrete, T_Strategy> {
        using Extractor = LogicKernelExtractor<LogicID, T_Concrete, T_Strategy>;
        // Safely provide a dummy size (1) if not a stencil, logic will be pruned by `is_valid`
        static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; 
        using Type = StaticStencilView<T_Concrete, T_Strategy, PointCount>;
        static constexpr bool is_valid = Extractor::has_kernel;
    };

    // AOSOA Views
    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width();
        using Type = AOSOAView<T_Concrete, LaneWidth, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width();
        using Type = AOSOAView<T_Concrete, LaneWidth, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width();
        using Type = AOSOAView<T_Concrete, LaneWidth, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width();
        using Type = AOSOAView<T_Concrete, LaneWidth, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width();
        using Type = AOSOAView<T_Concrete, LaneWidth, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    template <TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct ViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, T_Concrete, T_Strategy> {
        static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width();
        using Type = AOSOAView<T_Concrete, LaneWidth, T_Strategy>;
        static constexpr bool is_valid = true;
    };

    // --- The Recursive Factory Loop ---
    template<size_t L, size_t V, size_t S, size_t T>
    struct TransformFactoryMatrix {
        static void fill(std::array<TransformTaskFactoryFn, TransformTaskRegistry::TOTAL_COMBINATIONS>& arr) {
            constexpr TransformLogicID LogicEnum = static_cast<TransformLogicID>(L);
            constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
            constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
            constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

            using Traits = NativeMemoryTraits<MemTypeEnum>;
            using ConcreteType = typename Traits::ConcreteType;

            // Resolve Strategy O(1)
            using T_Strategy = typename StrategyResolver<StrategyEnum>::Type;

            // Resolve Logic & View
            using ResolvedLogic = LogicResolver<LogicEnum, ConcreteType, T_Strategy>;
            using ResolvedView = ViewResolver<ViewEnum, LogicEnum, MemTypeEnum, ConcreteType, T_Strategy>;

            // Initial Filter Pass: Only proceed if Logics and Views consider the combo theoretically valid
            constexpr bool combo_valid = ResolvedLogic::is_valid && ResolvedView::is_valid;

            // Secondary Filter Pass: Verify type bitmasks and hardware layout logic
            constexpr bool is_fully_valid = []() consteval {
                if constexpr (!combo_valid) return false;
                else {
                    constexpr bool is_type_supported = (static_cast<uint64_t>(ResolvedLogic::Type::supported_types) & static_cast<uint64_t>(Traits::DataFlag)) != 0;
                    if constexpr (!is_type_supported) return false;
                    else {
                        return TransformLogicValidator::validate(
                            ResolvedLogic::Type::requirements, 
                            ResolvedLogic::Type::supported_layouts, 
                            ViewTraits<typename ResolvedView::Type>::capabilities, 
                            BufferLayoutType::NONE
                        );
                    }
                }
            }();

            constexpr size_t flat_idx = 
                L * (TransformTaskRegistry::V_COUNT * TransformTaskRegistry::S_COUNT * TransformTaskRegistry::T_COUNT) +
                V * (TransformTaskRegistry::S_COUNT * TransformTaskRegistry::T_COUNT) +
                S * (TransformTaskRegistry::T_COUNT) +
                T;

            if constexpr (is_fully_valid) {
                arr[flat_idx] = []() -> INativeTask* {
                    return new TransformTask<typename ResolvedLogic::Type, typename ResolvedView::Type, T_Strategy>();
                };
            }

            // --- Multi-Dimensional Recursion ---
            if constexpr (T + 1 < TransformTaskRegistry::T_COUNT) {
                TransformFactoryMatrix<L, V, S, T + 1>::fill(arr);
            } else if constexpr (S + 1 < TransformTaskRegistry::S_COUNT) {
                TransformFactoryMatrix<L, V, S + 1, 0>::fill(arr);
            } else if constexpr (V + 1 < TransformTaskRegistry::V_COUNT) {
                TransformFactoryMatrix<L, V + 1, 0, 0>::fill(arr);
            } else if constexpr (L + 1 < TransformTaskRegistry::L_COUNT) {
                TransformFactoryMatrix<L + 1, 0, 0, 0>::fill(arr);
            }
        }
    };
} // end anonymous namespace

// Global static initialization
const std::array<TransformTaskFactoryFn, TransformTaskRegistry::TOTAL_COMBINATIONS> 
TransformTaskRegistry::factories = []{
    std::array<TransformTaskFactoryFn, TransformTaskRegistry::TOTAL_COMBINATIONS> arr;
    arr.fill(nullptr);
    TransformFactoryMatrix<0, 0, 0, 0>::fill(arr);
    return arr;
}();

godot::Dictionary* TransformTaskRegistry::ui_transform_matrix = nullptr;

void TransformTaskRegistry::init() {
    ui_transform_matrix = new godot::Dictionary();
}

void TransformTaskRegistry::cleanup() {
    delete ui_transform_matrix;
    ui_transform_matrix = nullptr;
}

std::unique_ptr<INativeTask> TransformTaskRegistry::create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    size_t flat_idx = 
        p_logic_id * (V_COUNT * S_COUNT * T_COUNT) +
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    TransformTaskFactoryFn factory = factories[flat_idx];
    if (factory) {
        return std::unique_ptr<INativeTask>(factory());
    }

    return nullptr;
}

} // namespace ideam::core