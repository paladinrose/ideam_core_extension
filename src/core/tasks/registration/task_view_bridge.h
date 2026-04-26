#pragma once

#include "native_task_registry.h"       // Gives us MemoryStrategy
#include "../../memory/views/view_traits.h" // Gives us ViewStrategies

namespace ideam::core {

/**
 * Task View Bridge
 * Safely maps Task-layer dispatch enums to Memory-layer bitmasks 
 * without violating the strict one-way dependency graph.
 */
[[nodiscard]] consteval ViewStrategies to_view_strategy_mask(MemoryStrategy s) noexcept {
    switch (s) {
        case MemoryStrategy::FlatStrategy:      return ViewStrategies::FLAT;
        case MemoryStrategy::SoAStrategy:       return ViewStrategies::SOA;
        case MemoryStrategy::AoSStrategy:       return ViewStrategies::AOS;
        case MemoryStrategy::Spatial2DStrategy: return ViewStrategies::SPATIAL_2D;
        case MemoryStrategy::Spatial3DStrategy: return ViewStrategies::SPATIAL_3D;
        case MemoryStrategy::Spatial4DStrategy: return ViewStrategies::SPATIAL_4D;
        case MemoryStrategy::TiledSoAStrategy:  return ViewStrategies::TILED_SOA;
        case MemoryStrategy::RingStrategy:      return ViewStrategies::RING;
        case MemoryStrategy::PagedStrategy:     return ViewStrategies::PAGED;
        default:                                return ViewStrategies::NONE;
    }
}

} // namespace ideam::core