#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <vector>

#include "ideam_graph_edit.h"
#include "ideam_graph_resource.h"
#include "theme_registry.h"

namespace ideam::godot_ext {

class GraphComposer : public godot::VBoxContainer {
    GDCLASS(GraphComposer, godot::VBoxContainer)

private:
    godot::HBoxContainer* header_bar = nullptr;
    godot::OptionButton* theme_selector = nullptr;
    godot::Button* load_theme_btn = nullptr;
    godot::Button* save_btn = nullptr;
    godot::TabContainer* tab_container = nullptr;
    godot::FileDialog* theme_file_dialog = nullptr;

    godot::Ref<ThemeRegistry> theme_registry;

    // DOD-Optimized State Tracker
    // Size: 24 bytes. Fits perfectly in 32-byte alignment.
    struct alignas(32) ActiveSession {
        const IdeamGraphResource* resource_key; 
        IdeamGraphEdit* editor_node;
        int tab_index;
        int theme_index; // Tracks the dropdown index (0 = Default)
    };
    
    std::vector<ActiveSession> active_sessions;

    // Internal State Management
    void _refresh_theme_list();

protected:
    static void _bind_methods();
    void _notification(int p_what);
    void _apply_default_composer_theme();

    static godot::Window* create_runtime_composer_window();

public:
    GraphComposer();
    ~GraphComposer();

    // Signal Handlers
    void _on_tab_changed(int p_tab);
    void _on_theme_selected(int p_index);
    void _on_load_theme_pressed();
    void _on_theme_file_selected(const godot::String& p_path);
    void _on_save_pressed();
    
    // Instance-level operations
    void open_graph(IdeamGraphEdit* p_graph_edit);
    void close_graph(IdeamGraphEdit* p_graph_edit);

    // Unified Static API routing
    static void edit_ideam_graph(IdeamGraphEdit* p_graph_edit, godot::Control* p_owner = nullptr);
    static void close_ideam_graph(IdeamGraphEdit* p_graph_edit, godot::Control* p_owner = nullptr);
};

} // namespace ideam::godot_ext