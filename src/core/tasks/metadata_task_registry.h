#ifndef IDEAM_CORE_METADATA_TASK_REGISTRY_H
#define IDEAM_CORE_METADATA_TASK_REGISTRY_H

#include "i_native_task.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates (The O(1) Indices) ---

enum class MetadataLogicID : uint32_t {
    DSUCluster = 0,
    GroupMask = 1,
    LOD = 2,
    Partition = 3,
    Count
};

enum class MetadataViewID : uint32_t {
    SingleElement = 0,
    StaticStencil = 1,
    Count
};

enum class MetadataStrategyID : uint32_t {
    Flat = 0,
    Spatial2D = 1,
    Spatial3D = 2,
    Spatial4D = 3, 
    Count
};

// Raw function pointer for zero-overhead instantiation
// Note: We return raw pointers from the factory function to keep the .rdata clean, 
// then immediately wrap them in std::unique_ptr at the API boundary.
using MetadataTaskFactoryFn = INativeTask* (*)();

class MetadataTaskRegistry {
public:
    // --- The O(1) Multi-Dimensional Factory Matrix ---
    // Declared here, but strictly defined and populated via template metaprogramming 
    // inside the .cpp file's anonymous namespace to maintain the TU Firewall.
    static const std::array<MetadataTaskFactoryFn, 
    static_cast<size_t>(MetadataLogicID::Count) * static_cast<size_t>(MetadataViewID::Count) * static_cast<size_t>(MetadataStrategyID::Count)> factories;

    // --- UX Layer ---
    static godot::Dictionary* ui_metadata_matrix;

    // --- Lifecycle ---
    static void init();
    static void cleanup();

    // --- O(1) Instantiation ---
    // Replaces the godot::StringName hashmap lookup.
    [[nodiscard]] static std::unique_ptr<INativeTask> create(
        uint32_t p_logic_id, 
        uint32_t p_view_id, 
        uint32_t p_strategy_id
    );
};

} // namespace ideam::core

#endif // IDEAM_CORE_METADATA_TASK_REGISTRY_H