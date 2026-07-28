#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/button.hpp>
#include <vector>

#include "ideam_graph_edit.h"
#include "ideam_graph_resource.h"
#include "../controls/theme_selector.h" 

namespace ideam::godot_ext {

class GraphComposer : public godot::VBoxContainer {
    GDCLASS(GraphComposer, godot::VBoxContainer)

private:
    godot::HBoxContainer* header_bar = nullptr;
    ThemeSelector* theme_selector_ui = nullptr; // Replaces previous dropdown/buttons/dialog/registry
    godot::Button* save_btn = nullptr;
    godot::TabContainer* tab_container = nullptr;

    // DOD-Optimized State Tracker
    struct alignas(32) ActiveSession {
        const IdeamGraphResource* resource_key; 
        IdeamGraphEdit* editor_node;
        int tab_index;
        int theme_index; // Tracks the dropdown index (0 = Default)
    };
    
    std::vector<ActiveSession> active_sessions;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
    // Updated to accept a theme directly
    void _apply_default_composer_theme(const godot::Ref<godot::Theme>& p_theme);

    static godot::Window* create_runtime_composer_window();

public:
    GraphComposer();
    ~GraphComposer();

    // Signal Handlers
    void _on_tab_changed(int p_tab);
    void _on_theme_applied(const godot::Ref<godot::Theme>& p_theme, int p_index);
    void _on_save_pressed();
    
    // Instance-level operations
    void open_graph(IdeamGraphEdit* p_graph_edit);
    void close_graph(IdeamGraphEdit* p_graph_edit);

    // Unified Static API routing
    static void edit_ideam_graph(IdeamGraphEdit* p_graph_edit, godot::Control* p_owner = nullptr);
    static void close_ideam_graph(IdeamGraphEdit* p_graph_edit, godot::Control* p_owner = nullptr);
};

} // namespace ideam::godot_ext