#pragma once

#include "gameplay_resource.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: Accessibility options typically represent a set of global or 
 * per-player state toggles and scaling factors (e.g., UI scale, high-contrast mode). 
 * While currently implemented as individual Resources (AoS), a Data-Oriented 
 * approach would consolidate these into a "Configuration Block" (SoA). 
 * * By storing these as a flat struct of scalars and bitfields, systems can 
 * check accessibility flags in a branchless manner, and the CPU can cache 
 * the entire configuration in a single line, eliminating the need to 
 * traverse multiple Resource pointers at runtime.
 */
class GameplayAccessibilityOption : public GameplayResource {
    GDCLASS(GameplayAccessibilityOption, GameplayResource)

protected:
    static void _bind_methods();

public:
    GameplayAccessibilityOption();
    ~GameplayAccessibilityOption();

    /**
     * DOD NOTE: This placeholder for application logic currently exists at the 
     * object level. In a performance-optimized system, this would be handled 
     * by a System that applies offsets or bitwise masks to globally 
     * registered accessibility states[cite: 111].
     */
    void apply_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_other);
};

} // namespace ideam::godot_ext