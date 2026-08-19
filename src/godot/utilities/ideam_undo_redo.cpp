#include "ideam_undo_redo.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_interface.hpp>
#endif
namespace ideam::godot_ext {
IdeamUndoRedo::IdeamUndoRedo() {
    // At runtime, we instantiate our own standard UndoRedo tracker.
    // In the editor, EditorUndoRedoManager is accessed globally/per-scene, 
    // so we don't necessarily need a standalone runtime instance allocated there 
    // (though keeping one as a safe fallback is fine).
    runtime_undo_redo = memnew(UndoRedo);
}

IdeamUndoRedo::~IdeamUndoRedo() {
    if (runtime_undo_redo) {
        memdelete(runtime_undo_redo);
        runtime_undo_redo = nullptr;
    }
}

void IdeamUndoRedo::_bind_methods() {
    // Bind methods if you want them exposed to GDScript, 
    // or keep them purely C++ for your GDExtension tools.
}

#ifdef TOOLS_ENABLED
void IdeamUndoRedo::set_editor_undo_redo(EditorUndoRedoManager *p_manager) {
    editor_undo_redo = p_manager;
}
#endif

void IdeamUndoRedo::create_action(const String &name, UndoRedo::MergeMode merge_mode, Object *custom_context) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            // If context is provided, pass it. Otherwise, let the editor manage the active scene context.
            if (custom_context) {
                editor_undo_redo->create_action(name, merge_mode, custom_context);
            } else {
                editor_undo_redo->create_action(name, merge_mode);
            }
            return;
        }
    }
#endif

    // Fallback to standard runtime UndoRedo
    if (runtime_undo_redo) {
        runtime_undo_redo->create_action(name, merge_mode);
    }
}

void IdeamUndoRedo::add_do_method(const Callable &callable) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_do_method(callable);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_do_method(callable);
    }
}

void IdeamUndoRedo::add_undo_method(const Callable &callable) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_undo_method(callable);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_undo_method(callable);
    }
}

void IdeamUndoRedo::add_do_property(Object *object, const StringName &property, const Variant &value) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_do_property(object, property, value);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_do_property(object, property, value);
    }
}

void IdeamUndoRedo::add_undo_property(Object *object, const StringName &property, const Variant &value) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->add_undo_property(object, property, value);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->add_undo_property(object, property, value);
    }
}

void IdeamUndoRedo::commit_action(bool execute) {
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (editor_undo_redo) {
            editor_undo_redo->commit_action(execute);
            return;
        }
    }
#endif
    if (runtime_undo_redo) {
        runtime_undo_redo->commit_action(execute);
    }
}
} // namespace ideam::godot_ext