#ifndef IDEAM_MEMORY_PLUGIN_H
#define IDEAM_MEMORY_PLUGIN_H

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/editor_inspector_plugin.hpp>

namespace godot {

class IdeamMemoryPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamMemoryPlugin, IdeamEditorPlugin)

private:
    static IdeamMemoryPlugin *singleton;

    // Ecosystem resource paths defining memory layout defaults
    static constexpr std::string_view MEMORY_SETTINGS_PATH = "res://addons/ideam_memory/resources/project_memory_settings.res";

    // Inspector for Memory resources (BufferPODs, GrantPODs)
    Ref<EditorInspectorPlugin> memory_inspector;

protected:
    static void _bind_methods();

public:
    IdeamMemoryPlugin();
    virtual ~IdeamMemoryPlugin() override;
    
    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // Global Access
    static Object *undo_redo();
};

} // namespace godot

#endif // IDEAM_MEMORY_PLUGIN_H