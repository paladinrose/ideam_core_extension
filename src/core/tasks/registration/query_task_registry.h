#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"
#include "ideam_task_registry.h"
#include "../query_task.h" // Supplies QueryOp
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
    ColorRGBA,
    ColorHSVA,
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
    // --- DOD Matrix Bounds (4D Sub-Matrix) ---
    static constexpr size_t O_COUNT = 2; // 0 = CULL, 1 = ADD
    static constexpr size_t L_COUNT = static_cast<size_t>(QueryLogicID::Count);
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);
    
    static constexpr size_t SUB_MATRIX_SIZE = O_COUNT * V_COUNT * S_COUNT * T_COUNT;

    using SubMatrix = std::array<QueryTaskFactoryFn, SUB_MATRIX_SIZE>;

    // --- The O(1) Multi-Dimensional Factory Routers ---
    static std::array<const SubMatrix*, L_COUNT> logic_matrices;
    
    /**
     * ui_query_matrix Data Schema (Exported to Godot):
     * {
     * "LogicID (Stringified Int)": {
     * "properties": [ Array of godot::Dictionary (PropertyInfo structures from Logic::get_ui_properties) ],
     * "valid_combinations": PackedInt64Array [ FlatIdx hashes of valid Op/View/Strategy/Type combinations ]
     * }
     * }
     */
    //static godot::Dictionary ui_query_matrix;

    // --- Lifecycle Management ---
    
    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing();
    static void cleanup_execution_routing();

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI (Call ONLY when baking manifest)
    static void generate_ui_matrices(godot::Dictionary& p_matrix);
    
    // Direct O(1) Fetch
    static std::unique_ptr<INativeTask> create(uint32_t p_op_id, uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id);
};

} // namespace ideam::core