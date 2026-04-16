#ifndef IDEAM_CORE_QUERY_TASK_REGISTRY_H
#define IDEAM_CORE_QUERY_TASK_REGISTRY_H

#include "i_native_task.h"
#include "query_logic/query_logic_traits.h"
#include "../memory/memory_common.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates (The 4D O(1) Indices) ---
enum class QueryLogicID : uint32_t {
    AABB = 0,
    Archetype,
    Bitmask,
    Boolean,
    Border,
    Color,
    Component,
    DataComparison,
    DataRange,
    Directional,
    Distance,
    EventRingBridge,
    Frustum,
    HierarchicalBridge,
    Limit,
    Morphological,
    PagedToTiledBridge,
    Predicate,
    RelationalBridge,
    SpatialInclusionBridge,
    SpatialProjectionBridge,
    StencilDilationBridge,
    Stochastic,
    SwapEruptionBridge,
    Count
};

enum class QueryViewID : uint32_t {
    SingleElement = 0,
    Count
};

enum class QueryStrategyID : uint32_t {
    Flat = 0,
    Spatial2D,
    Spatial3D,
    Count
};

// Raw function pointer for zero-overhead instantiation
using QueryTaskFactoryFn = INativeTask* (*)();

class QueryTaskRegistry {
public:
    // --- DOD Matrix Bounds (4 Dimensions) ---
    static constexpr size_t L_COUNT = static_cast<size_t>(QueryLogicID::Count);
    static constexpr size_t O_COUNT = 2; // QueryOp::CULL (0) and QueryOp::ADD (1)
    static constexpr size_t V_COUNT = static_cast<size_t>(QueryViewID::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(QueryStrategyID::Count);
    
    static constexpr size_t TOTAL_COMBINATIONS = L_COUNT * O_COUNT * V_COUNT * S_COUNT;

    // --- The O(1) Multi-Dimensional Factory Matrix ---
    static const std::array<QueryTaskFactoryFn, TOTAL_COMBINATIONS> factories;

    // --- UX Layer ---
    static godot::Dictionary* ui_query_matrix;

    // --- Lifecycle ---
    static void init();
    static void cleanup();

    // --- O(1) Instantiation ---
    // Note: p_op_id is 0 for CULL, 1 for ADD
    static std::unique_ptr<INativeTask> create(uint32_t p_logic_id, uint32_t p_op_id, uint32_t p_view_id, uint32_t p_strategy_id);
};

} // namespace ideam::core

#endif // IDEAM_CORE_QUERY_TASK_REGISTRY_H