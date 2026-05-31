#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"
#include "ideam_task_registry.h" 
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates (The O(1) Indices) ---
enum class TransformLogicID : uint32_t {
    BoundaryConstraint = 0,
    BoundsExtraction,
    DataScatter,
    DataSort,
    EulerIntegration,
    FastNoiseLite,
    NoiseInjection,
    Stencil_Moore_R1,
    Stencil_Moore_R2,
    Stencil_Moore_R3,
    Stencil_VonNeumann_R1,
    Stencil_VonNeumann_R2,
    Stencil_VonNeumann_R3,
    ValueAccumulation,
    Count
};

using TransformTaskFactoryFn = INativeTask* (*)();

class TransformTaskRegistry {
public:
    static constexpr size_t L_COUNT = static_cast<size_t>(TransformLogicID::Count);
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);
    
    // --- DOD Matrix Bounds (3D Sub-Matrix) ---
    static constexpr size_t SUB_MATRIX_SIZE = V_COUNT * S_COUNT * T_COUNT;

    using SubMatrix = std::array<TransformTaskFactoryFn, SUB_MATRIX_SIZE>;

    // --- The O(1) Multi-Dimensional Factory Routers ---
    static std::array<const SubMatrix*, L_COUNT> logic_matrices;
    
    /**
     * ui_transform_matrix Data Schema (Exported to Godot):
     * {
     * "LogicID (Stringified Int)": {
     * "properties": [ Array of godot::Dictionary (PropertyInfo structures from Logic::get_ui_properties) ],
     * "valid_combinations": PackedInt64Array [ FlatIdx hashes of valid View/Strategy/Type combinations ]
     * }
     * }
     */
    static godot::Dictionary* ui_transform_matrix;

    // --- Lifecycle Management ---
    
    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing();
    static void cleanup_execution_routing();

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI (Call ONLY when baking manifest)
    static void generate_ui_matrices();
    static void cleanup_ui_matrices();

    static std::unique_ptr<INativeTask> create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id);
};

} // namespace ideam::core