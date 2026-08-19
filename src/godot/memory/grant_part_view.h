#pragma once

#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

class GrantPartView : public godot::PanelContainer {
    GDCLASS(GrantPartView, godot::PanelContainer)

private:
    godot::GridContainer* grid = nullptr;
    godot::Label* buffer_id_label = nullptr;
    godot::Label* access_mode_label = nullptr;
    godot::Label* element_count_label = nullptr;
    godot::Label* element_stride_label = nullptr;
    godot::Label* capacity_label = nullptr;

protected:
    static void _bind_methods();

public:
    GrantPartView();
    ~GrantPartView() = default;

    void populate(const godot::Dictionary& p_data);
};

} // namespace ideam::godot_ext