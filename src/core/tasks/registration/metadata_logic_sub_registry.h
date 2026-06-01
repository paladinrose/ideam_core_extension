#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

#include "metadata_task_registry.h"
#include "task_view_bridge.h"
#include "../metadata_task.h"
#include "../../memory/views/view_traits.h"
// --- Logics ---
#include "../metadata_logic/dsu_cluster_metadata_logic.h"
#include "../metadata_logic/group_mask_metadata_logic.h"
#include "../metadata_logic/lod_metadata_logic.h"
#include "../metadata_logic/partition_metadata_logic.h"

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

    template <MetadataLogicID ID, typename T_Concrete, typename T_Strategy> struct MetadataLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::DSUCluster_Moore_R1, C, S>  { using Type = DSUClusterMetadataLogic<C, 9>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::DSUCluster_VonNeumann_R1, C, S> { using Type = DSUClusterMetadataLogic<C, 5>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::GroupMask_1Bit, C, S> { using Type = GroupMaskMetadataLogic<C, 1>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::GroupMask_2Bit, C, S> { using Type = GroupMaskMetadataLogic<C, 2>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::GroupMask_3Bit, C, S> { using Type = GroupMaskMetadataLogic<C, 3>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::GroupMask_4Bit, C, S> { using Type = GroupMaskMetadataLogic<C, 4>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::LOD_1Level, C, S> { using Type = LODMetadataLogic<C, 1>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::LOD_2Level, C, S> { using Type = LODMetadataLogic<C, 2>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::LOD_3Level, C, S> { using Type = LODMetadataLogic<C, 3>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::LOD_4Level, C, S> { using Type = LODMetadataLogic<C, 4>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::Partition_1, C, S> { using Type = PartitionMetadataLogic<C, 1>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::Partition_2, C, S> { using Type = PartitionMetadataLogic<C, 2>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::Partition_3, C, S> { using Type = PartitionMetadataLogic<C, 3>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct MetadataLogicResolver<MetadataLogicID::Partition_4, C, S> { using Type = PartitionMetadataLogic<C, 4>; static constexpr bool is_valid = true; };

    template <MetadataLogicID LogicID, typename C, typename S> struct LogicKernelExtractor : KernelExtractorImpl<MetadataLogicResolver<LogicID, C, S>> {};

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

    template <MemoryView ViewID, MetadataLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy> struct MetadataViewResolver { static constexpr bool is_valid = false; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = StrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = LogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
}

template <MetadataLogicID L>
struct MetadataLogicSubRegistry {
    static std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE> factories;

    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing() {
        _fill_matrix<false>(std::make_index_sequence<MetadataTaskRegistry::SUB_MATRIX_SIZE>{}, nullptr);
    }

    static void cleanup_execution_routing() {
        factories.fill(nullptr);
    }

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI
    static void generate_ui_matrices(godot::Dictionary& p_matrix) {
        _fill_matrix<true>(std::make_index_sequence<MetadataTaskRegistry::SUB_MATRIX_SIZE>{}, &p_matrix);
    }

private:
    template <bool BuildUI, size_t... Indices>
    static void _fill_matrix(std::index_sequence<Indices...>, godot::Dictionary* p_matrix) {
        (_fill_single<BuildUI, Indices>(p_matrix), ...);
    }

    template <bool BuildUI, size_t FlatIdx>
    static void _fill_single(godot::Dictionary* p_matrix) {
        constexpr size_t T_COUNT = MetadataTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = MetadataTaskRegistry::S_COUNT;

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

        using ResolvedLogic = MetadataLogicResolver<L, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedLogic::is_valid) return;
        using L_Type = typename ResolvedLogic::Type;

        using ResolvedView = MetadataViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
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

            // 3. The 6-Argument Core Contract Validation
            return MetadataLogicValidator::validate(
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
            if constexpr (!BuildUI) {
                // Hot-Path Execution Routing
                factories[FlatIdx] = []() -> INativeTask* {
                    return new MetadataTask<L_Type, V_Type, T_Strategy>();
                };
            } else {
                // Cold-Path UI Collection
                if (p_matrix) {
                    godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));
                    
                    if (!p_matrix->has(logic_key)) {
                        godot::Dictionary dict;
                        // Instantiate the property array exactly once per logic struct type
                        dict["properties"] = L_Type::get_ui_properties();
                        dict["valid_combinations"] = godot::PackedInt64Array(); 
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

template <MetadataLogicID L>
std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE> 
MetadataLogicSubRegistry<L>::factories = []{
    std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core