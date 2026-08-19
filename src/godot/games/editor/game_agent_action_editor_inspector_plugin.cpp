#include "game_agent_action_editor_inspector_plugin.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

// Assuming these are available in your C++ architecture
#include "../game_entities/actions/game_agent_action.h"
#include "../ui/game_agent_action_sequencer.h" 

namespace ideam::godot_ext {

void GameAgentActionEditorInspectorPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_editor_root", "root"), &GameAgentActionEditorInspectorPlugin::set_editor_root);
    godot::ClassDB::bind_method(godot::D_METHOD("get_editor_root"), &GameAgentActionEditorInspectorPlugin::get_editor_root);
    
    // Bind the property so the main EditorPlugin can assign it freely
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "editor_root", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_editor_root", "get_editor_root");
}

GameAgentActionEditorInspectorPlugin::GameAgentActionEditorInspectorPlugin() {}

GameAgentActionEditorInspectorPlugin::~GameAgentActionEditorInspectorPlugin() {}

void GameAgentActionEditorInspectorPlugin::set_editor_root(godot::Control* p_root) {
    editor_root = p_root;
}

godot::Control* GameAgentActionEditorInspectorPlugin::get_editor_root() const {
    return editor_root;
}

bool GameAgentActionEditorInspectorPlugin::_can_handle(godot::Object *p_object) {
    // Safely check if the object is our GameAgentAction
    return godot::Object::cast_to<GameAgentAction>(p_object) != nullptr;
}

bool GameAgentActionEditorInspectorPlugin::_parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) {
    
    if (p_name == "sequence") {
        // Instantiate our custom UI sequencer
        GameAgentActionSequencer* s = memnew(GameAgentActionSequencer);
        

#ifdef TOOLS_ENABLED
        Object *undo_redo = get_undo_redo();
        s->set("undo_redo", undo_redo);
        s->set("editor_root", editor_root);
#endif

        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(p_object);
        if (action) {
            // Again, using call() allows us to interact seamlessly even if 
            // the sequencer is still a GDScript under the hood for now.
            s->call("set_action_to_sequence", action);
        }
        
        add_custom_control(s);
        
        // Return true to tell the inspector to hide the default property editor
        // and use our custom control entirely.
        return true;
    }
    
    return false;
}

} // namespace ideam::godot_ext