#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"
#include "native_task_registry.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates (The O(1) Indices) ---
enum class MetadataLogicID : uint32_t {
    DSUCluster_Moore_R1 = 0,
    DSUCluster_VonNeumann_R1,
    
    GroupMask_1Bit,
    GroupMask_2Bit,
    GroupMask_3Bit,
    GroupMask_4Bit,
    
    LOD_1Level,
    LOD_2Level,
    LOD_3Level,
    LOD_4Level,
    
    Partition_1,
    Partition_2,
    Partition_3,
    Partition_4,
    
    Count
};

using MetadataTaskFactoryFn = INativeTask* (*)();

class MetadataTaskRegistry {
public:
    static constexpr size_t L_COUNT = static_cast<size_t>(MetadataLogicID::Count);
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);
    
    // --- DOD Matrix Bounds (3D Sub-Matrix) ---
    // Metadata tasks lack the 'QueryOp' dimension.
    static constexpr size_t SUB_MATRIX_SIZE = V_COUNT * S_COUNT * T_COUNT;

    using SubMatrix = std::array<MetadataTaskFactoryFn, SUB_MATRIX_SIZE>;

    // --- The O(1) Multi-Dimensional Factory Routers ---
    static std::array<const SubMatrix*, L_COUNT> logic_matrices;
    static godot::Dictionary* ui_metadata_matrix;

    static void init();
    static void cleanup();

    static std::unique_ptr<INativeTask> create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id);
};

} // namespace ideam::core