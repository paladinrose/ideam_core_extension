#pragma once

#include "gameplay_resource.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: GameplayPerspective typically manages camera offsets, FOV, and 
 * view-dependent logic. While inheriting from godot::Resource allows for 
 * flexible data-driven design, it creates a "chunky" object profile. 
 *
 * In a high-performance C++26 renderer or gameplay loop, these perspectives 
 * should be refactored into a "View Component" stored in a contiguous 
 * Structure of Arrays (SoA). By processing view data as raw scalars, the 
 * CPU can update camera transitions and culling logic across multiple 
 * players or viewports using SIMD vectorization, bypassing the overhead of 
 * Resource pointer dereferencing.
 */
class GameplayPerspective : public GameplayResource {
    GDCLASS(GameplayPerspective, GameplayResource)

protected:
    static void _bind_methods();

public:
    GameplayPerspective();
    ~GameplayPerspective();

    /**
     * DOD NOTE: This method currently acts as an object-level state copy. 
     * In a Data-Oriented architecture, perspective blending should be 
     * handled by a System that performs linear interpolation (LERP) 
     * across raw data buffers, rather than invoking virtual methods 
     * on discrete Resource instances.
     */
    void apply_perspective(const godot::Ref<GameplayPerspective> &p_other);
};

} // namespace ideam::godot_ext