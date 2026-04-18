#pragma once

#include "i_native_task.h"
#include "../memory/memory_common.h"
#include "native_task_registry.h"
#include "query_task.h" // Supplies QueryOp
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates (The O(1) Indices) ---
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

using QueryTaskFactoryFn = INativeTask* (*)();

class QueryTaskRegistry {
public:
    // --- DOD Matrix Bounds (5D) ---
    // QueryTask requires QueryOp (CULL/ADD) as a template parameter. 
    static constexpr size_t O_COUNT = 2; // 0 = CULL, 1 = ADD
    static constexpr size_t L_COUNT = static_cast<size_t>(QueryLogicID::Count);
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);
    
    static constexpr size_t TOTAL_COMBINATIONS = O_COUNT * L_COUNT * V_COUNT * S_COUNT * T_COUNT;

    // --- The O(1) Multi-Dimensional Factory Matrix ---
    static const std::array<QueryTaskFactoryFn, TOTAL_COMBINATIONS> factories;

    static godot::Dictionary* ui_query_matrix;

    static void init();
    static void cleanup();

    // Direct O(1) Fetch
    static std::unique_ptr<INativeTask> create(uint32_t p_op_id, uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id);
};

} // namespace ideam::core

 // IDEAM_CORE_QUERY_TASK_REGISTRY_H