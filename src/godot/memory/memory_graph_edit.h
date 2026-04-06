#ifndef IDEAM_GODOT_MEMORY_GRAPH_EDIT_H
#define IDEAM_GODOT_MEMORY_GRAPH_EDIT_H

#include "../graphs/ideam_graph_edit.h"
#include "../graphs/ideam_graph_resource.h"
#include "memory_inspectors.h"
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <unordered_map>

namespace godot {

/**
 * @class MemoryGraphEdit
 * @brief Specialized GraphEdit for Memory topologies.
 * Handles reverse-lookup telemetry routing from the C++ DOD backend to the UI,
 * and context-aware port signature filtering for rapid graphing.
 */
class MemoryGraphEdit : public IdeamGraphEdit {
    GDCLASS(MemoryGraphEdit, IdeamGraphEdit)

private:
    // Reverse lookup map: Core NodeID -> UI StringName
    std::unordered_map<uint32_t, StringName> dod_to_ui_map;

    // Specialized popup for filtered context menus
    PopupMenu* filtered_popup = nullptr;
    uint32_t current_filter_mask = 0;
    Vector2 memory_popup_position;

protected:
    static void _bind_methods();

    void _create_filtered_popup();
    
    // Signal handler for drawing lines into empty space
    void _on_connection_to_empty(const StringName &p_from_node, int p_from_port, const Vector2 &p_release_position);
    
    // Display the filtered creation menu
    void _show_filtered_popup(const Vector2 &p_at, uint32_t p_filter_mask);
    
    // Handle selection from the filtered popup
    void _filtered_popup_select(int p_id);

    // Virtual method to get the list of memory-specific nodes, filtered by trait mask
    virtual TypedArray<String> _get_filtered_node_types(uint32_t p_filter_mask) const;

public:
    MemoryGraphEdit();
    virtual ~MemoryGraphEdit() override;

    void _ready() override;

    // --- Telemetry Routing ---
    void set_telemetry_mapping(const Dictionary& p_map);
    void push_telemetry(int p_core_id, const Ref<ideam::godot_ext::MemoryGrantInspector>& p_inspector);
};

} // namespace godot

#endif // IDEAM_GODOT_MEMORY_GRAPH_EDIT_H