#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgent;

class GameAgentEditorInspectorPlugin : public IdeamEditorInspectorPlugin {
    GDCLASS(GameAgentEditorInspectorPlugin, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

private:
    GameAgent* agent = nullptr;
    godot::Window* _tool_window = nullptr;

public:
    GameAgentEditorInspectorPlugin();
    ~GameAgentEditorInspectorPlugin();

    // Base overrides
    virtual godot::Object *get_undo_redo() const override;
    virtual bool _can_handle(godot::Object *p_object) override;
    
    // Utilizing the strict signature we fixed earlier!
    virtual bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override;

    // Custom Methods
    void _on_new_action_button_pressed();
    void open_new_action_tool_window(GameAgent* p_agent, GameAgentEditorInspectorPlugin* p_editor);
    void new_action_tool_close();

    void _node_retargeter_change(godot::Object* new_retargeter);
    void _signal_connector_change(godot::Object* new_connector);
};

} // namespace ideam::godot_ext