#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

using namespace godot;

namespace ideam::godot_ext {
class IdeamUndoRedo : public RefCounted {
    GDCLASS(IdeamUndoRedo, RefCounted);

private:
    UndoRedo *runtime_undo_redo = nullptr;
    
#ifdef TOOLS_ENABLED
    EditorUndoRedoManager *editor_undo_redo = nullptr;
#endif

protected:
    static void _bind_methods();

public:
    IdeamUndoRedo();
    ~IdeamUndoRedo();

#ifdef TOOLS_ENABLED
    void set_editor_undo_redo(EditorUndoRedoManager *p_manager);
#endif

    // Unified API
    void create_action(const String &name, UndoRedo::MergeMode merge_mode = UndoRedo::MERGE_DISABLE, Object *custom_context = nullptr);
    void add_do_method(const Callable &callable);
    void add_undo_method(const Callable &callable);
    void add_do_property(Object *object, const StringName &property, const Variant &value);
    void add_undo_property(Object *object, const StringName &property, const Variant &value);
    void commit_action(bool execute = true);
};
} // namespace ideam::godot_ext