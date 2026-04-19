#pragma once

#include "metadata_task_registry.h"
#include "../metadata_task.h"
#include "../../memory/views/view_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

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
    template <MetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct MetadataViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<C, S>; static constexpr bool is_valid = true; };
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

    template <typename T_Array>
    inline void _append_unique(T_Array& p_array, const godot::String& p_val) {
        if (!p_array.has(p_val)) {
            p_array.push_back(p_val);
        }
    }
}

template <MetadataLogicID L>
struct MetadataLogicSubRegistry {
    static std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE> factories;

    static void init() {
        SubMatrixBuilder<0, 0, 0>::fill(factories);
    }

    static void cleanup() {
        factories.fill(nullptr);
    }

private:
    template <size_t V, size_t S, size_t T>
    struct SubMatrixBuilder {
        static void fill(std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE>& arr) {
            constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
            constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
            constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

            using Traits = NativeMemoryTraits<MemTypeEnum>;
            using ConcreteType = typename Traits::ConcreteType;
            using ResolvedStrategy = StrategyResolver<StrategyEnum>;
            using T_Strategy = typename ResolvedStrategy::Type;
            using ResolvedLogic = MetadataLogicResolver<L, ConcreteType, T_Strategy>;
            using ResolvedView = MetadataViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;

            constexpr bool combo_valid = ResolvedStrategy::is_valid && ResolvedLogic::is_valid && ResolvedView::is_valid;

            constexpr bool is_fully_valid = []() consteval {
                if constexpr (!combo_valid) return false;
                else {
                    constexpr bool is_type_supported = (static_cast<uint64_t>(ResolvedLogic::Type::supported_types) & static_cast<uint64_t>(Traits::DataFlag)) != 0;
                    if constexpr (!is_type_supported) return false;
                    else {
                        return MetadataLogicValidator::validate(
                            ResolvedLogic::Type::requirements, 
                            ResolvedLogic::Type::supported_layouts, 
                            ViewTraits<typename ResolvedView::Type>::capabilities, 
                            BufferLayoutType::NONE
                        );
                    }
                }
            }();

            constexpr size_t flat_idx = 
                V * (MetadataTaskRegistry::S_COUNT * MetadataTaskRegistry::T_COUNT) + 
                S * (MetadataTaskRegistry::T_COUNT) + T;

            if constexpr (is_fully_valid) {
                arr[flat_idx] = []() -> INativeTask* {
                    return new MetadataTask<typename ResolvedLogic::Type, typename ResolvedView::Type, T_Strategy>(typename ResolvedLogic::Type{});
                };

                if (MetadataTaskRegistry::ui_metadata_matrix) {
                    godot::String logic_name(ResolvedLogic::Type::type_name);
                    if (!MetadataTaskRegistry::ui_metadata_matrix->has(logic_name)) {
                        godot::Dictionary dict;
                        dict["views"] = godot::Array(); 
                        dict["strategies"] = godot::Array();
                        (*MetadataTaskRegistry::ui_metadata_matrix)[logic_name] = dict;
                    }
                    godot::Dictionary dict = (*MetadataTaskRegistry::ui_metadata_matrix)[logic_name];
                    _append_unique<godot::Array>(dict["views"], ResolvedView::Type::type_name);
                    _append_unique<godot::Array>(dict["strategies"], T_Strategy::type_name);
                }
            }

            // 3D Recursive Loop
            if constexpr (T + 1 < MetadataTaskRegistry::T_COUNT) {
                SubMatrixBuilder<V, S, T + 1>::fill(arr);
            } else if constexpr (S + 1 < MetadataTaskRegistry::S_COUNT) {
                SubMatrixBuilder<V, S + 1, 0>::fill(arr);
            } else if constexpr (V + 1 < MetadataTaskRegistry::V_COUNT) {
                SubMatrixBuilder<V + 1, 0, 0>::fill(arr);
            }
        }
    };
};

template <MetadataLogicID L>
std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE> 
MetadataLogicSubRegistry<L>::factories = []{
    std::array<MetadataTaskFactoryFn, MetadataTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core