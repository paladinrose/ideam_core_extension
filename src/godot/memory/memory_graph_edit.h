#pragma once

#include "../graphs/ideam_graph_edit.h"
#include "memory_graph_resource.h"
#include "memory_inspectors.h"
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <unordered_map>

namespace ideam::godot_ext {

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
    std::unordered_map<uint32_t, godot::StringName> dod_to_ui_map;

    // Specialized popup for filtered context menus
    godot::PopupMenu* filtered_popup = nullptr;
    uint32_t current_filter_mask = 0;
    godot::Vector2 memory_popup_position;

protected:
    static void _bind_methods();

    void _create_filtered_popup();
    
    // Signal handler for drawing lines into empty space
    void _on_connection_to_empty(const godot::StringName &p_from_node, int p_from_port, const godot::Vector2 &p_release_position);
    
    // Display the filtered creation menu
    void _show_filtered_popup(const godot::Vector2 &p_at, uint32_t p_filter_mask);
    
    // Handle selection from the filtered popup
    void _filtered_popup_select(int p_id);

    // Virtual method to get the list of memory-specific nodes, filtered by trait mask
    virtual godot::TypedArray<godot::String> _get_filtered_node_types(uint32_t p_filter_mask) const;

public:
    MemoryGraphEdit();
    virtual ~MemoryGraphEdit() override;

    void _ready() override;

    // --- Telemetry Routing ---
    // Called by MemoryGraphHost/DOD core to map execution IDs back to UI node names
    void set_telemetry_mapping(const godot::Dictionary& p_map);

    // Broadcasts an active memory grant snapshot to the specific visual node
    void push_telemetry(int p_core_id, const godot::Ref<MemoryGrantInspector>& p_inspector);
};

} // namespace ideam::godot_ext

 // IDEAM_GODOT_MEMORY_GRAPH_EDIT_H