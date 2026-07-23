#include "memory_manager_inspector.h"
#include "memory_manager_resource.h"
// Include the header where MemoryProfiler is defined
#include "memory_profiler.h" 
// Replace with the actual plugin that manages this context if different from IdeamTasksPlugin
#include "ideam_memory_plugin.h" 

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryManagerInspector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_open_profiler_pressed", "object"), &MemoryManagerInspector::_on_open_profiler_pressed);
}

MemoryManagerInspector::MemoryManagerInspector() {}
MemoryManagerInspector::~MemoryManagerInspector() {}

Object *MemoryManagerInspector::get_undo_redo() const {
    // Modify this if your memory tools rely on a different plugin for undo/redo actions
    return IdeamMemoryPlugin::undo_redo();
}

bool MemoryManagerInspector::_can_handle(Object *p_object) {
    if (!p_object) return false;
    // Strictly handle MemoryManagerResource objects
    return Object::cast_to<MemoryManagerResource>(p_object) != nullptr;
}

void MemoryManagerInspector::_parse_begin(Object *p_object) {
    Button *open_button = memnew(Button);
    open_button->set_text("Profile Memory");

    Array args;
    args.append(p_object);
    open_button->connect("pressed", Callable(this, "_on_open_profiler_pressed").bindv(args));
    
    add_custom_control(open_button);
}

bool MemoryManagerInspector::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
    return false;
}

void MemoryManagerInspector::_on_open_profiler_pressed(Object* p_object) {
    auto* raw_resource = Object::cast_to<MemoryManagerResource>(p_object);
    if (!raw_resource) return;

    // Secure the resource as a Ref before passing it along
    Ref<MemoryManagerResource> memory_manager_ref(raw_resource);
    
    // Pass the reference directly to the static profiler method
    MemoryProfiler::profile_memory_manager(memory_manager_ref);
}

} // namespace ideam::godot_ext