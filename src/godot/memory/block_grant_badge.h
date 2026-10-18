#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class BlockGrantBadge : public godot::Button {
    GDCLASS(BlockGrantBadge, godot::Button)

private:
    int grant_index = -1;
    int access_mode = 0; // 0 = READ, 1 = WRITE
    bool has_error = false;

    void _update_appearance();

    void _on_pressed();
    void _on_mouse_entered();
    void _on_mouse_exited();

protected:
    static void _bind_methods();

public:
    BlockGrantBadge();
    ~BlockGrantBadge() = default;

    void configure(int p_grant_index, const godot::StringName& p_grant_name, int p_access_mode, bool p_has_error, const godot::String& p_tooltip_text = godot::String());

    int get_grant_index() const { return grant_index; }
    int get_access_mode() const { return access_mode; }
    bool get_has_error() const { return has_error; }
};

} // namespace ideam::godot_ext