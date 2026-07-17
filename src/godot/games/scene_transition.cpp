#include "scene_transition.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void SceneTransition::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("transition_started"));
    ADD_SIGNAL(godot::MethodInfo("transition_completed"));
    ADD_SIGNAL(godot::MethodInfo("to_scene_not_found"));
    ADD_SIGNAL(godot::MethodInfo("load_to_scene_started"));
    ADD_SIGNAL(godot::MethodInfo("load_to_scene_failed"));
    ADD_SIGNAL(godot::MethodInfo("load_to_scene_completed", godot::PropertyInfo(godot::Variant::OBJECT, "to_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node")));

    // Internal state properties (Optional to expose, but useful for generic querying)
    godot::ClassDB::bind_method(godot::D_METHOD("get_to_path"), &SceneTransition::get_to_path);

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("start_transition", "to_path", "from"), &SceneTransition::start_transition, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("transition"), &SceneTransition::transition);
    godot::ClassDB::bind_method(godot::D_METHOD("to_scene_load_complete"), &SceneTransition::to_scene_load_complete);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_transition"), &SceneTransition::complete_transition);

    //bind_methods and ADD_PROPERTY, get/set _from
    //bind_methods and ADD_PROPERTY, get/set to_path
    //bind_methods and ADD_PROPERTY, get/set _to

}

SceneTransition::SceneTransition() {}

SceneTransition::~SceneTransition() {}

void SceneTransition::_process(double delta) {
    if (_transition_in_progress) {
        godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
        // Assuming path is valid if transition is in progress. String hashing occurs here.
        if (loader->load_threaded_get_status(_to_path) == godot::ResourceLoader::THREAD_LOAD_LOADED) {
            to_scene_load_complete();
        }
    }
}

// Getters / Setters
void SceneTransition::set_transition_in_progress(bool p_in_progress) {
    if (p_in_progress == _transition_in_progress) return;
    _transition_in_progress = p_in_progress; 
}
bool SceneTransition::get_transition_in_progress() const { return _transition_in_progress; }

void SceneTransition::set_from(godot::Node* p_from) { 
    if (p_from == _from) return;
    _from = p_from; 
    // emit_signal
}
godot::Node* SceneTransition::get_from() const { return _from; }

void SceneTransition::set_to_path(const godot::String& p_to_path) { 
    if (p_to_path == _to_path) return;
    _to_path = p_to_path; 
    // emit_signal
}
godot::String SceneTransition::get_to_path() const { return _to_path; }

void SceneTransition::set_to(godot::Node* p_to) { 
    if (p_to == _to) return;
    _to = p_to; 
    // emit_signal
}
godot::Node* SceneTransition::get_to() const { return _to; }

// Class Functions
void SceneTransition::start_transition(const godot::String& to_path, godot::Node* from) {
    if (_transition_in_progress) {
        return;
    }
    
    _from = nullptr;
    _to_path = "";
    _to = nullptr;
    
    if (to_path.is_empty()) {
        return;
    }
        
    _from = from;
    _to_path = to_path;
    
    _transition_in_progress = true;
    
    if (get_signal_connection_list("transition_started").size() > 0) {
        emit_signal("transition_started");
    } else {
        transition();
    }
}

void SceneTransition::transition() {
    if (!_transition_in_progress) {
        return;
    }
    
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    
    if (!loader->exists(_to_path)) {
        emit_signal("to_scene_not_found");
        return;
    }
        
    godot::ResourceLoader::ThreadLoadStatus status = loader->load_threaded_get_status(_to_path);
    if (status == godot::ResourceLoader::THREAD_LOAD_IN_PROGRESS) {
        godot::UtilityFunctions::print("Already loading");
        return;
    }
        
    loader->load_threaded_request(_to_path);
    emit_signal("load_to_scene_started");
}

void SceneTransition::to_scene_load_complete() {
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    godot::Ref<godot::PackedScene> loadedResource = loader->load_threaded_get(_to_path);
    
    if (!loadedResource.is_valid()) {
        return;
    }
    
    _to = loadedResource->instantiate();
    
    if (!_to) {
        emit_signal("load_to_scene_failed");
        return;
    }
    
    if (get_signal_connection_list("load_to_scene_completed").size() > 0) {
        emit_signal("load_to_scene_completed", _to);
    } else {
        complete_transition();
    }
}

void SceneTransition::complete_transition() {
    _transition_in_progress = false;
    
    emit_signal("transition_completed");
    
    _from = nullptr;
    _to = nullptr;
    _to_path = "";
}

} // namespace ideam::godot_ext