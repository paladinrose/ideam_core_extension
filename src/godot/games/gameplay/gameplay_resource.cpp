#include "gameplay_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GameplayResource::_bind_methods() {
    // Property Binding [cite: 110]
    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &GameplayResource::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &GameplayResource::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");
}

GameplayResource::GameplayResource() {
    // DOD NOTE: Keeping constructor overhead minimal to support fast 
    // instantiation during scene/resource loading.
}

GameplayResource::~GameplayResource() {
}

void GameplayResource::set_title(const godot::String &p_title) {
    if (p_title == title) return;
    title = p_title;
    emit_changed();
}

godot::String GameplayResource::get_title() const {
    return title;
}

} // namespace ideam::godot_ext