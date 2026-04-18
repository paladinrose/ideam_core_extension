#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgentAction;

class GameAgentActionEditorInspectorPlugin : public IdeamEditorInspectorPlugin {
    GDCLASS(GameAgentActionEditorInspectorPlugin, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

private:
    godot::Control* editor_root = nullptr;

public:
    GameAgentActionEditorInspectorPlugin();
    ~GameAgentActionEditorInspectorPlugin();

    // Editor Root injection
    void set_editor_root(godot::Control* p_root);
    godot::Control* get_editor_root() const;

    // Base overrides
    virtual bool _can_handle(godot::Object *p_object) override;
    
    // Utilizing the strict signature!
    virtual bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override;
};

} // namespace ideam::godot_ext