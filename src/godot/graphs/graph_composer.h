#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <vector>

#include "ideam_graph_edit.h"
#include "ideam_graph_resource.h"

namespace ideam::godot_ext {

class GraphComposer : public godot::VBoxContainer {
    GDCLASS(GraphComposer, godot::VBoxContainer)

private:
    godot::TabContainer* tab_container = nullptr;

    // DOD-Optimized State Tracker
    // Size: 24 bytes. Padding to 32 bytes via alignas ensures optimal fit 
    // for AVX operations and prevents false sharing on cache lines.
    struct alignas(32) ActiveSession {
        // We store the raw pointer as a fast-comparison key to avoid Ref<> ref-count thrashing
        const IdeamGraphResource* resource_key; 
        IdeamGraphEdit* editor_node;
        int tab_index;
    };
    
    // Contiguous memory array for fast linear scans without pointer-chasing.
    std::vector<ActiveSession> active_sessions;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    GraphComposer();
    ~GraphComposer();

    // Instance-level operations (supports localized, non-singleton usage)
    void open_graph(IdeamGraphEdit* p_graph_edit);
    void close_graph(IdeamGraphEdit* p_graph_edit);

    // Unified Static API routing
    static void edit_ideam_graph(IdeamGraphEdit* p_graph_edit, godot::Control* p_owner = nullptr);
    static void close_ideam_graph(IdeamGraphEdit* p_graph_edit, godot::Control* p_owner = nullptr);
};

} // namespace ideam::godot_ext

 // IDEAM_GRAPH_COMPOSER_H