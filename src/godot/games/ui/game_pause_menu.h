#pragma once

#include <godot_cpp/classes/control.hpp>

namespace ideam::godot_ext {

class GamePauseMenu : public godot::Control {
    GDCLASS(GamePauseMenu, godot::Control)

protected:
    static void _bind_methods();

public:
    GamePauseMenu();
    ~GamePauseMenu();

    // Godot Functions
    // virtual void _ready() override;
    // virtual void _process(double delta) override;
};

} // namespace ideam::godot_ext