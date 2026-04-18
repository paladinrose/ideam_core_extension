// transform_task_registry.h
#pragma once

#include "i_native_task.h"
#include "../memory/memory_common.h"
#include "native_task_registry.h" // Assumptions: Provides MemoryView, MemoryStrategy, MemoryTypes, and NativeMemoryTraits
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

// Raw function pointer for zero-overhead instantiation
using TransformTaskFactoryFn = INativeTask* (*)();

class TransformTaskRegistry {
public:
    // --- DOD Matrix Bounds ---
    static constexpr size_t L_COUNT = static_cast<size_t>(TransformLogicID::Count);
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);
    
    static constexpr size_t TOTAL_COMBINATIONS = L_COUNT * V_COUNT * S_COUNT * T_COUNT;

    // --- The O(1) Multi-Dimensional Factory Matrix ---
    static const std::array<TransformTaskFactoryFn, TOTAL_COMBINATIONS> factories;

    // --- UX Layer ---
    static godot::Dictionary* ui_transform_matrix;

    static void init();
    static void cleanup();

    [[nodiscard]] static std::unique_ptr<INativeTask> create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id);
};

} // namespace ideam::core

 // IDEAM_CORE_TRANSFORM_TASK_REGISTRY_H